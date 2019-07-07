// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/ILog.h>
#include <CryThreading/CryAtomics.h>
#include <CryThreading/MultiThread_Containers.h>
#include <CryThreading/IThreadManager.h>
#include <3rdParty/concqueue/concqueue-mpsc.hpp>

struct ISystem;
struct ICVar;
struct IConsole;
struct IConsoleCmdArgs;

//////////////////////////////////////////////////////////////////////

#define MAX_FILENAME_SIZE 256

#if defined(DEDICATED_SERVER) || defined (CRY_PLATFORM_WINDOWS) || (defined(CRY_PLATFORM_DURANGO) && !defined(EXCLUDE_NORMAL_LOG))
	#define KEEP_LOG_FILE_OPEN 1
#else
	#define KEEP_LOG_FILE_OPEN 0
#endif

class CLogThread : public IThread
{
public:

	CLogThread(concqueue::mpsc_queue_t<string>& logQueue);
	virtual ~CLogThread();
	virtual void ThreadEntry() override;
	void SignalStopWork();
	void SignalStartWork();
	void AddLog(const string& str) { m_logQueue.enqueue(str); }
	bool IsRunning() const         { return m_bIsRunning; }

private:

	bool m_bIsRunning = false;
	concqueue::mpsc_queue_t<string>& m_logQueue;
};

struct SExclusiveThreadAccessLock
{
	SExclusiveThreadAccessLock() : counter(0), writerThreadId(0) {}
	volatile uint32   counter;
	volatile threadID writerThreadId;
};

//////////////////////////////////////////////////////////////////////
class CLog : public ILog, public ISystemEventListener
{
public:
	typedef std::list<ILogCallback*>   Callbacks;
	typedef CryStackStringT<char, 256> LogStringType;

	// constructor
	CLog(ISystem* pSystem);
	// destructor
	~CLog();

	// interface ISystemEventListener -------------------------------------------

	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

	// interface ILog, IMiniLog -------------------------------------------------

	virtual void        Release() { delete this; };
	virtual bool        SetFileName(const char* filename);
	virtual const char* GetFileName() const;
	virtual const char* GetFilePath() const;
	virtual const char* GetBackupFilePath() const;
#if !defined(EXCLUDE_NORMAL_LOG)
	virtual void        Log(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogAlways(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogWarning(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogError(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogPlus(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogToFile(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogToFilePlus(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogToConsole(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        LogToConsolePlus(const char* command, ...) PRINTF_PARAMS(2, 3);
#else
	virtual void        Log(const char* command, ...) PRINTF_PARAMS(2, 3)              {};
	virtual void        LogAlways(const char* command, ...) PRINTF_PARAMS(2, 3)        {};
	virtual void        LogWarning(const char* command, ...) PRINTF_PARAMS(2, 3)       {};
	virtual void        LogError(const char* command, ...) PRINTF_PARAMS(2, 3)         {};
	virtual void        LogPlus(const char* command, ...) PRINTF_PARAMS(2, 3)          {};
	virtual void        LogToFile(const char* command, ...) PRINTF_PARAMS(2, 3)        {};
	virtual void        LogToFilePlus(const char* command, ...) PRINTF_PARAMS(2, 3)    {};
	virtual void        LogToConsole(const char* command, ...) PRINTF_PARAMS(2, 3)     {};
	virtual void        LogToConsolePlus(const char* command, ...) PRINTF_PARAMS(2, 3) {};
#endif // !defined(EXCLUDE_NORMAL_LOG)
	virtual void        UpdateLoadingScreen(const char* command, ...) PRINTF_PARAMS(2, 3);
	virtual void        SetVerbosity(int verbosity);
	virtual int         GetVerbosityLevel() const;
	virtual void        RegisterConsoleVariables();
	virtual void        UnregisterConsoleVariables();
	virtual void        AddCallback(ILogCallback* pCallback);
	virtual void        RemoveCallback(ILogCallback* pCallback);
	virtual void        LogV(const ELogType ineType, int flags, const char* szFormat, va_list args);
	virtual void        LogV(const ELogType ineType, const char* szFormat, va_list args);
	virtual void        Update();
	virtual const char* GetModuleFilter();
	virtual void        Flush();
	virtual void        FlushAndClose();

	virtual void        SetLogMode(ELogMode eLogMode);
	virtual ELogMode    GetLogMode() const;

	virtual void        ThreadExclusiveLogAccess(bool state);
	
	void				SetLogFormat(const char* format);

private: // -------------------------------------------------------------------

#if !defined(EXCLUDE_NORMAL_LOG)
	void LogStringToFile(const char* szString, bool bAdd, bool bError = false);
	void LogStringToConsole(const char* szString, bool bAdd = false);
#else
	void LogStringToFile(const char* szString, bool bAdd, bool bError = false) {};
	void LogStringToConsole(const char* szString, bool bAdd = false)           {};
#endif // !defined(EXCLUDE_NORMAL_LOG)

	FILE* OpenLogFile(const char* filename, const char* mode);
	void  CloseLogFile(bool force = false);

	// will format the message into m_szTemp
	void FormatMessage(const char* szCommand, ...) PRINTF_PARAMS(2, 3);
	void FormatTimestampInternal(stack_string& timeStr, const string & logFormat);

#if defined(SUPPORT_LOG_IDENTER)
	void                Indent(CLogIndenter* indenter);
	void                Unindent(CLogIndenter* indenter);
	void                BuildIndentString();
	virtual void        PushAssetScopeName(const char* sAssetType, const char* sName);
	virtual void        PopAssetScopeName();
	virtual const char* GetAssetScopeString();
#endif

	static void OnUseLogThreadChange(ICVar* var);

	CLogThread*                     m_pLogThread;
	ICVar*                          m_pUseLogThread;
	concqueue::mpsc_queue_t<string> m_logQueue;
	bool                            m_bIsPostSystemInit = false;

	ISystem*                  m_pSystem;
	float                     m_fLastLoadingUpdateTime; // for non-frequent streamingEngine update
	string                    m_filename;               // Contains only the file name and the extension
	string                    m_filePath;               // Contains the full absolute path to the log file
	string                    m_backupFilePath;         // Contains the full absolute path to the backup log file
	FILE*                     m_pLogFile = nullptr;
	CryStackStringT<char, 32> m_LogMode;                // mode m_pLogFile has been opened with
	FILE*                     m_pErrFile = nullptr;
	int                       m_nErrCount = 0;
	string                    m_logFormat;              // Time logging format

#if defined(SUPPORT_LOG_IDENTER)
	uint8               m_indentation = 0;
	LogStringType       m_indentWithString;
	class CLogIndenter* m_topIndenter = nullptr;

	struct SAssetScopeInfo
	{
		string sType;
		string sName;
	};

	std::vector<SAssetScopeInfo> m_assetScopeQueue;
	CryCriticalSection           m_assetScopeQueueLock;
	string                       m_assetScopeString;
#endif

	ICVar*             m_pLogIncludeTime = nullptr;
	IConsole*          m_pConsole = nullptr;

	CryCriticalSection m_logCriticalSection;

	struct SLogHistoryItem
	{
		LogStringType str;
		const char*   ptr;
		ELogType      type;
		float         time;

		SLogHistoryItem()
			: ptr(nullptr)
			, type(eMessage)
			, time(0.0f) {}
	};
	SLogHistoryItem m_history[16];
	int             m_iLastHistoryItem = 0;

#if KEEP_LOG_FILE_OPEN
	static void LogFlushFile(IConsoleCmdArgs* pArgs);

	bool m_bFirstLine = true;
	char m_logBuffer[256 * 1024];
#endif

public: // -------------------------------------------------------------------

	void GetMemoryUsage(ICrySizer* pSizer) const;
	// checks the verbosity of the message and returns NULL if the message must NOT be
	// logged, or the pointer to the part of the message that should be logged
	const char* CheckAgainstVerbosity(const char* pText, bool& logtofile, bool& logtoconsole, const uint8 DefaultVerbosity = 2);

	// create backup of log file, useful behavior - only on development platform
	void CreateBackupFile();

	ICVar*    m_pLogVerbosity = nullptr;                            //
	ICVar*    m_pLogWriteToFile = nullptr;                          //
	ICVar*    m_pLogWriteToFileVerbosity = nullptr;                 //
	ICVar*    m_pLogVerbosityOverridesWriteToFile = nullptr;        //
	ICVar*    m_pLogSpamDelay = nullptr;                            //
	ICVar*    m_pLogModule = nullptr;                               // Module filter for log
	Callbacks m_callbacks;                                //

	threadID  m_nMainThreadId;

	struct SLogMsg
	{
		LogStringType msg;
		bool          bError;
		bool          bAdd;
		bool          bConsole;
		void          GetMemoryUsage(ICrySizer* pSizer) const {}
	};
	CryMT::queue<SLogMsg>              m_threadSafeMsgQueue;

	ELogMode                           m_eLogMode = eLogMode_Normal;
	mutable SExclusiveThreadAccessLock m_exclusiveLogFileThreadAccessLock;
};
