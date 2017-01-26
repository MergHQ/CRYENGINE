// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#if defined(ENABLE_LOADING_PROFILER)

	#include "BootProfiler.h"
	#include "ThreadInfo.h"
	#include <CryThreading/IThreadManager.h>

namespace
{
	#define CACHE_LINE_SIZE_BP_INTERNAL 64

enum
{
	eMAX_THREADS_TO_PROFILE = 64,
	eNUM_RECORDS_PER_POOL   = 4680,   // so, eNUM_RECORDS_PER_POOL * sizeof(CBootProfilerRecord) == mem consumed by pool item
	// poolmem ~= 1Mb for 1 pool per thread
};

class CBootProfilerThreadsInterface
{
public:
	CBootProfilerThreadsInterface()
	{
		Reset();
	}

	void Reset()
	{
		m_threadCounter = 0;
		memset(m_threadInfo, 0, sizeof(m_threadInfo));
		for (int i = 0; i < eMAX_THREADS_TO_PROFILE; ++i)
		{
			m_threadNames[i].clear();
		}
	}

	ILINE unsigned int GetThreadIndexByID(threadID threadID)
	{
		for (int i = 0; i < eMAX_THREADS_TO_PROFILE; ++i)
		{
			if (m_threadInfo[i] == 0)
				break;
			if (m_threadInfo[i] == threadID)
				return i;
		}

		const unsigned int counter = CryInterlockedIncrement(&m_threadCounter) - 1;     //count to index
		m_threadInfo[counter] = threadID;
		m_threadNames[counter] = gEnv->pThreadManager->GetThreadName(threadID);

		return counter;
	}

	ILINE const char* GetThreadNameByIndex(unsigned int threadIndex)
	{
		assert(threadIndex < m_threadCounter);
		return m_threadNames[threadIndex].c_str();
	}

	int GetThreadCount() const
	{
		return m_threadCounter;
	}

private:
	threadID            m_threadInfo[eMAX_THREADS_TO_PROFILE];                    //threadIDs
	CryFixedStringT<32> m_threadNames[eMAX_THREADS_TO_PROFILE];
	int                 m_threadCounter;
};

CBootProfiler gProfilerInstance;
CBootProfilerThreadsInterface gThreadsInterface;
}

int CBootProfiler::CV_sys_bp_frames = 0;
float CBootProfiler::CV_sys_bp_frames_threshold = 0.0f;
float CBootProfiler::CV_sys_bp_time_threshold = 0.0f;

class CProfileBlockTimes
{
protected:
	LARGE_INTEGER m_startTimeStamp;
	LARGE_INTEGER m_stopTimeStamp;
	CProfileBlockTimes()
	{
		memset(&m_startTimeStamp, 0, sizeof(m_startTimeStamp));
		memset(&m_stopTimeStamp, 0, sizeof(m_stopTimeStamp));
	}
};

class CRecordPool;
class CBootProfilerSession;

class CBootProfilerRecord
{
public:
	const char*          m_label;
	LARGE_INTEGER        m_startTimeStamp;
	LARGE_INTEGER        m_stopTimeStamp;

	unsigned int         m_threadIndex;

	CBootProfilerRecord* m_pParent;

	CBootProfilerRecord* m_pFirstChild;
	CBootProfilerRecord* m_pLastChild;
	CBootProfilerRecord* m_pNextSibling;

	CryFixedStringT<256> m_args;

	ILINE CBootProfilerRecord(const char* label, LARGE_INTEGER timestamp, unsigned int threadIndex, const char* args) :
		m_label(label), m_startTimeStamp(timestamp), m_threadIndex(threadIndex), m_pParent(nullptr), m_pFirstChild(nullptr), m_pLastChild(nullptr), m_pNextSibling(nullptr)
	{
		memset(&m_stopTimeStamp, 0, sizeof(m_stopTimeStamp));
		if (args)
			m_args = args;
	}

	void AddChild(CBootProfilerRecord* pRecord)
	{
		if (!m_pFirstChild)
			m_pFirstChild = pRecord;

		if (m_pLastChild)
		{
			m_pLastChild->m_pNextSibling = pRecord;
		}

		m_pLastChild = pRecord;
		pRecord->m_pParent = this;
	}

	void Print(FILE* file, char* buf, size_t buf_size, size_t depth, LARGE_INTEGER stopTime, LARGE_INTEGER frequency, const char* threadName, const float timeThreshold)
	{
		if (m_stopTimeStamp.QuadPart == 0)
			m_stopTimeStamp = stopTime;

		const float time = (float)(m_stopTimeStamp.QuadPart - m_startTimeStamp.QuadPart) * 1000.f / (float)frequency.QuadPart;

		if (timeThreshold > 0.0f && time < timeThreshold)
		{
			return;
		}

		string tabs; //tabs(depth++, '\t')
		tabs.insert(0, depth++, '\t');

		{
			string label = m_label;
			label.replace("&", "&amp;");
			label.replace("<", "&lt;");
			label.replace(">", "&gt;");
			label.replace("\"", "&quot;");
			label.replace("'", "&apos;");

			if (m_args.size() > 0)
			{
				m_args.replace("&", "&amp;");
				m_args.replace("<", "&lt;");
				m_args.replace(">", "&gt;");
				m_args.replace("\"", "&quot;");
				m_args.replace("'", "&apos;");
			}

			cry_sprintf(buf, buf_size, "%s<block name=\"%s\" totalTimeMS=\"%f\" startTime=\"%" PRIu64 "\" stopTime=\"%" PRIu64 "\" args=\"%s\"> \n",
			            tabs.c_str(), label.c_str(), time, m_startTimeStamp.QuadPart, m_stopTimeStamp.QuadPart, m_args.c_str());
			fprintf(file, "%s", buf);
		}

		for (CBootProfilerRecord* pRecord = m_pFirstChild; pRecord; pRecord = pRecord->m_pNextSibling)
		{
			pRecord->Print(file, buf, buf_size, depth, m_stopTimeStamp, frequency, threadName, timeThreshold);
		}

		cry_sprintf(buf, buf_size, "%s</block>\n", tabs.c_str());
		fprintf(file, "%s", buf);
	}
};

//////////////////////////////////////////////////////////////////////////

class CRecordPool
{
public:
	CRecordPool() : m_baseAddr(nullptr), m_allocCounter(0), m_next(nullptr)
	{
		m_baseAddr = (CBootProfilerRecord*)CryModuleMemalign(eNUM_RECORDS_PER_POOL * sizeof(CBootProfilerRecord), 16);
	}

	~CRecordPool()
	{
		CryModuleMemalignFree(m_baseAddr);
		delete m_next;
	}

	ILINE CBootProfilerRecord* allocateRecord()
	{
		if (m_allocCounter < eNUM_RECORDS_PER_POOL)
		{
			CBootProfilerRecord* newRecord = m_baseAddr + m_allocCounter;
			++m_allocCounter;
			return newRecord;
		}
		else
		{
			return nullptr;
		}
	}

	ILINE void setNextPool(CRecordPool* pool) { m_next = pool; }

private:
	CBootProfilerRecord* m_baseAddr;
	uint32               m_allocCounter;

	CRecordPool*         m_next;
};

class CBootProfilerSession : protected CProfileBlockTimes
{
public:
	CBootProfilerSession(const char* szName);
	~CBootProfilerSession();

	void                 Start();
	void                 Stop();

	CBootProfilerRecord* StartBlock(const char* name, const char* args, const unsigned int threadIndex);
	void                 StopBlock(CBootProfilerRecord* record);

	float                GetTotalTime() const
	{
		const float time = (float)(m_stopTimeStamp.QuadPart - m_startTimeStamp.QuadPart) * 1000.f / (float)m_frequency.QuadPart;
		return time;
	}

	void        CollectResults(const char* filename, const float timeThreshold);

	const char* GetName() const
	{
		return m_name.c_str();
	}

	static unsigned int sSessionCounter;

private:

	struct CRY_ALIGN(CACHE_LINE_SIZE_BP_INTERNAL) SThreadEntry
	{
		union
		{
			struct
			{
				CBootProfilerRecord* m_pRootRecord;
				CBootProfilerRecord* m_pCurrentRecord;
				CRecordPool*         m_pRecordsPool;
				unsigned int         m_totalBlocks;
			};
			volatile char m_padding[CACHE_LINE_SIZE_BP_INTERNAL];
		};
	};

	SThreadEntry        m_threadEntry[eMAX_THREADS_TO_PROFILE];

	CryFixedStringT<32> m_name;

	LARGE_INTEGER       m_frequency;
};

unsigned int CBootProfilerSession::sSessionCounter = 0;
//////////////////////////////////////////////////////////////////////////

CBootProfilerSession::CBootProfilerSession(const char* szName) : m_name(szName)
{
	memset(m_threadEntry, 0, sizeof(m_threadEntry));

	QueryPerformanceFrequency(&m_frequency);
}

CBootProfilerSession::~CBootProfilerSession()
{
	const unsigned int nThreadCount = gThreadsInterface.GetThreadCount();
	for (unsigned int i = 0; i < nThreadCount; ++i)
	{
		SThreadEntry& entry = m_threadEntry[i];
		delete entry.m_pRecordsPool;
	}

	++sSessionCounter;
}

void CBootProfilerSession::Start()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	m_startTimeStamp = time;
}

void CBootProfilerSession::Stop()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	m_stopTimeStamp = time;
}

CBootProfilerRecord* CBootProfilerSession::StartBlock(const char* name, const char* args, const unsigned int threadIndex)
{
	assert(threadIndex < eMAX_THREADS_TO_PROFILE);

	const unsigned int sessionIndex = sSessionCounter;

	SThreadEntry& entry = m_threadEntry[threadIndex];
	++entry.m_totalBlocks;

	if (!entry.m_pRootRecord)
	{
		CRecordPool* pPool = new CRecordPool;
		entry.m_pRecordsPool = pPool;

		CBootProfilerRecord* rec = pPool->allocateRecord();
		entry.m_pRootRecord = entry.m_pCurrentRecord = new(rec) CBootProfilerRecord("root", m_startTimeStamp, threadIndex, args);
	}

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	assert(entry.m_pRootRecord);

	CRecordPool* pPool = entry.m_pRecordsPool;
	assert(pPool);
	CBootProfilerRecord* rec = pPool->allocateRecord();
	if (!rec)
	{
		//pool is full, create a new one
		pPool = new CRecordPool;
		pPool->setNextPool(entry.m_pRecordsPool);
		entry.m_pRecordsPool = pPool;
		rec = pPool->allocateRecord();
	}

	CBootProfilerRecord* pNewRecord = new(rec) CBootProfilerRecord(name, time, threadIndex, args);
	entry.m_pCurrentRecord->AddChild(pNewRecord);
	entry.m_pCurrentRecord = pNewRecord;

	return pNewRecord;
}

void CBootProfilerSession::StopBlock(CBootProfilerRecord* record)
{
	if (record)
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		record->m_stopTimeStamp = time;
		assert(record->m_threadIndex < eMAX_THREADS_TO_PROFILE);

		SThreadEntry& entry = m_threadEntry[record->m_threadIndex];
		entry.m_pCurrentRecord = record->m_pParent;
	}
}

void CBootProfilerSession::CollectResults(const char* filename, const float timeThreshold)
{
	if (!(gEnv && gEnv->pCryPak))
		return;

	static const char* szTestResults = "%USER%/TestResults";
	string filePath = string(szTestResults) + "\\" + "bp_" + filename + ".xml";
	char path[ICryPak::g_nMaxPath] = "";
	gEnv->pCryPak->AdjustFileName(filePath.c_str(), path, ICryPak::FLAGS_PATH_REAL | ICryPak::FLAGS_FOR_WRITING);
	gEnv->pCryPak->MakeDir(szTestResults);

	FILE* pFile = ::fopen(path, "wb");
	if (!pFile)
		return; //TODO: use accessible path when punning from package on durango

	char buf[512];
	const unsigned int buf_size = sizeof(buf);

	cry_sprintf(buf, buf_size, "<root>\n");
	fprintf(pFile, "%s", buf);

	const size_t numThreads = gThreadsInterface.GetThreadCount();
	for (size_t i = 0; i < numThreads; ++i)
	{
		const SThreadEntry& entry = m_threadEntry[i];
		CBootProfilerRecord* pRoot = entry.m_pRootRecord;
		if (pRoot)
		{
			pRoot->m_stopTimeStamp = m_stopTimeStamp;

			const char* threadName = gThreadsInterface.GetThreadNameByIndex(i);
			if (!threadName)
				threadName = "UNKNOWN";

			const float time = (float)(pRoot->m_stopTimeStamp.QuadPart - pRoot->m_startTimeStamp.QuadPart) * 1000.f / (float)m_frequency.QuadPart;

			cry_sprintf(buf, buf_size, "\t<thread name=\"%s\" totalTimeMS=\"%f\" startTime=\"%" PRIu64 "\" stopTime=\"%" PRIu64 "\" totalBlocks=\"%u\"> \n", threadName, time,
			            pRoot->m_startTimeStamp.QuadPart, pRoot->m_stopTimeStamp.QuadPart, entry.m_totalBlocks);
			fprintf(pFile, "%s", buf);

			for (CBootProfilerRecord* pRecord = pRoot->m_pFirstChild; pRecord; pRecord = pRecord->m_pNextSibling)
			{
				pRecord->Print(pFile, buf, buf_size, 2, m_stopTimeStamp, m_frequency, threadName, timeThreshold);
			}

			cry_sprintf(buf, buf_size, "\t</thread>\n");
			fprintf(pFile, "%s", buf);
		}
	}

	cry_sprintf(buf, buf_size, "</root>\n");
	fprintf(pFile, "%s", buf);

	::fclose(pFile);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBootProfiler& CBootProfiler::GetInstance()
{
	return gProfilerInstance;
}

CBootProfiler::CBootProfiler()
	: m_pCurrentSession(nullptr)
	, m_pPreviousSession(nullptr)
	, m_pFrameRecord(nullptr)
	, m_levelLoadAdditionalFrames(0)
{
}

CBootProfiler::~CBootProfiler()
{
	if (gEnv && gEnv->pSystem)
	{
		ISystemEventDispatcher* pSystemEventDispatcher = gEnv->pSystem->GetISystemEventDispatcher();
		if (pSystemEventDispatcher)
		{
			pSystemEventDispatcher->RemoveListener(this);
		}
	}

	delete m_pCurrentSession;
}

// start session
void CBootProfiler::StartSession(const char* sessionName)
{
	if (m_pCurrentSession)
	{
		CryLogAlways("BootProfiler: failed to start session '%s' as another one is active '%s'", sessionName, m_pCurrentSession->GetName());
		return;
	}

	gThreadsInterface.Reset();
	CBootProfilerSession* pSession = new CBootProfilerSession(sessionName);
	pSession->Start();
	m_pCurrentSession = pSession;
}

// stop session
void CBootProfiler::StopSession()
{
	if (m_pCurrentSession)
	{
		gEnv->bBootProfilerEnabledFrames = false;

		CBootProfilerSession* session = m_pCurrentSession;
		m_pCurrentSession = nullptr;

		session->Stop();
		session->CollectResults(session->GetName(), CV_sys_bp_time_threshold);

		delete session;
	}
}

CBootProfilerRecord* CBootProfiler::StartBlock(const char* name, const char* args, unsigned int& sessionIndex)
{
	if (CBootProfilerSession* pSession = m_pCurrentSession)
	{
		const threadID curThread = CryGetCurrentThreadId();
		const unsigned int threadIndex = gThreadsInterface.GetThreadIndexByID(curThread);
		sessionIndex = CBootProfilerSession::sSessionCounter;
		return pSession->StartBlock(name, args, threadIndex);
	}
	return nullptr;
}

void CBootProfiler::StopBlock(CBootProfilerRecord* record, const unsigned int sessionIndex)
{
	if (m_pCurrentSession && sessionIndex == CBootProfilerSession::sSessionCounter)
	{
		m_pCurrentSession->StopBlock(record);
	}
}

void CBootProfiler::StartFrame(const char* name)
{
	static int prev_CV_sys_bp_frames = CV_sys_bp_frames;
	if (CV_sys_bp_frames > 0)
	{
		if (prev_CV_sys_bp_frames == 0)
		{
			StartSession("frames");
			gEnv->bBootProfilerEnabledFrames = true;
		}
		unsigned int dummy;
		m_pFrameRecord = StartBlock(name, nullptr, dummy);

		if (CV_sys_bp_frames_threshold != 0.0f) // we can't have 2 modes enabled at the same time
			CV_sys_bp_frames_threshold = 0.0f;
	}

	prev_CV_sys_bp_frames = CV_sys_bp_frames;

	static float prev_CV_sys_bp_frames_threshold = CV_sys_bp_frames_threshold;
	if (prev_CV_sys_bp_frames_threshold == 0.0f && CV_sys_bp_frames_threshold != 0.0f)
	{
		gThreadsInterface.Reset();
		gEnv->bBootProfilerEnabledFrames = true;
	}
	prev_CV_sys_bp_frames_threshold = CV_sys_bp_frames_threshold;

	if (CV_sys_bp_frames_threshold != 0.0f)
	{
		if (m_pCurrentSession)
		{
			CryLogAlways("BootProfiler: failed to start session 'frame_threshold' as another one is active '%s'", m_pCurrentSession->GetName());
		}
		else
		{
			CBootProfilerSession* pSession = new CBootProfilerSession("frame_threshold");
			pSession->Start();
			m_pCurrentSession = pSession;
			unsigned int dummy;
			m_pFrameRecord = StartBlock(name, nullptr, dummy);
		}
	}
}

void CBootProfiler::StopFrame()
{
	if (!m_pCurrentSession)
		return;

	if (CV_sys_bp_frames)
	{
		m_pCurrentSession->StopBlock(m_pFrameRecord);
		m_pFrameRecord = nullptr;

		--CV_sys_bp_frames;
		if (0 == CV_sys_bp_frames)
		{
			StopSession();
		}
	}

	if (m_levelLoadAdditionalFrames)
	{
		--m_levelLoadAdditionalFrames;
		if (0 == m_levelLoadAdditionalFrames)
		{
			StopSession();
		}
	}

	delete m_pPreviousSession;
	m_pPreviousSession = NULL;

	static float prev_CV_sys_bp_frames_threshold = CV_sys_bp_frames_threshold;
	const bool bDisablingThresholdMode = (prev_CV_sys_bp_frames_threshold > 0.0f && CV_sys_bp_frames_threshold == 0.0f);

	if (CV_sys_bp_frames_threshold > 0.0f || bDisablingThresholdMode)
	{
		CBootProfilerSession* pSession = m_pCurrentSession;

		pSession->StopBlock(m_pFrameRecord);
		m_pFrameRecord = nullptr;

		pSession->Stop();

		const float time = pSession->GetTotalTime();
		if ((time >= CV_sys_bp_frames_threshold) && !bDisablingThresholdMode)
		{
			const int frameNum = gEnv->pRenderer->GetFrameID(false);
			CryFixedStringT<32> sessionName;
			sessionName.Format("frame_%d_%3.1fms", frameNum, CV_sys_bp_frames_threshold);

			pSession->CollectResults(sessionName, CV_sys_bp_time_threshold);
		}

		m_pPreviousSession = pSession;
		m_pCurrentSession = nullptr;

		if (bDisablingThresholdMode)
		{
			gEnv->bBootProfilerEnabledFrames = false;
		}
	}

	prev_CV_sys_bp_frames_threshold = CV_sys_bp_frames_threshold;
}

void CBootProfiler::Init(ISystem* pSystem)
{
	pSystem->GetISystemEventDispatcher()->RegisterListener(this,"CBootProfiler");
	StartSession("boot");
}

void CBootProfiler::RegisterCVars()
{
	REGISTER_CVAR2("sys_bp_frames", &CV_sys_bp_frames, 0, VF_DEV_ONLY, "Starts frame profiling for specified number of frames using BootProfiler");
	REGISTER_CVAR2("sys_bp_frames_threshold", &CV_sys_bp_frames_threshold, 0, VF_DEV_ONLY, "Starts frame profiling but gathers the results for frames that frame time exceeded the threshold");
	REGISTER_CVAR2("sys_bp_time_threshold", &CV_sys_bp_time_threshold, 0.1f, VF_DEV_ONLY, "If greater than 0 don't write blocks that took less time (default 0.1 ms)");
}

void CBootProfiler::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
		{
			StopSession();
			break;
		}
	case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
		{
			break;
		}

	case ESYSTEM_EVENT_GAME_MODE_SWITCH_END:
		{
			break;
		}

	case ESYSTEM_EVENT_LEVEL_LOAD_START:
		{
			break;
		}
	case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
		{
			CV_sys_bp_time_threshold = 0.1f;
			StartSession("level");
			break;
		}
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			break;
		}
	case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
		{
			//level loading can be stopped here, or m_levelLoadAdditionalFrames can be used to prolong dump for this amount of frames
			StopSession();
			//m_levelLoadAdditionalFrames = 20;

			CV_sys_bp_time_threshold = 0.0f; //gather all blocks when in runtime
			break;
		}
	}
}

#endif
