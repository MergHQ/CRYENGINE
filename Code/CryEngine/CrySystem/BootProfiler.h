// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __bootprofilersystem_h__
#define __bootprofilersystem_h__
#pragma once

#if defined(ENABLE_LOADING_PROFILER)

class CBootProfilerRecord;
class CBootProfilerSession;

//////////////////////////////////////////////////////////////////////////

class CBootProfiler : public ISystemEventListener
{
	friend class CBootProfileBLock;
public:
	CBootProfiler();
	~CBootProfiler();

	static CBootProfiler& GetInstance();

	void                  Init(ISystem* pSystem);
	void                  RegisterCVars();

	void                  StartSession(const char* sessionName);
	void                  StopSession();

	CBootProfilerRecord*  StartBlock(const char* name, const char* args, unsigned int& sessionIndex);
	void                  StopBlock(CBootProfilerRecord* record, const unsigned int sessionIndex);

	void                  StartFrame(const char* name);
	void                  StopFrame();
protected:
	// === ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

private:
	CBootProfilerSession* m_pCurrentSession;

	static int            CV_sys_bp_frames;
	static float          CV_sys_bp_frames_threshold;
	static float          CV_sys_bp_time_threshold;
	CBootProfilerRecord*  m_pFrameRecord;

	int                   m_levelLoadAdditionalFrames;
};

#else //ENABLE_LOADING_PROFILER
#endif

#endif
