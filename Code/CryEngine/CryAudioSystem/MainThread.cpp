// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "MainThread.h"
#include "Common.h"
#include "System.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CMainThread::ThreadEntry()
{
	while (m_doWork)
	{
		g_system.InternalUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CMainThread::SignalStopWork()
{
	m_doWork = false;
}

///////////////////////////////////////////////////////////////////////////
void CMainThread::Activate()
{
	if (!gEnv->pThreadManager->SpawnThread(this, "AudioMainThread"))
	{
		CryFatalError(R"(Error spawning "AudioMainThread" thread.)");
	}
}

///////////////////////////////////////////////////////////////////////////
void CMainThread::Deactivate()
{
	SignalStopWork();
	gEnv->pThreadManager->JoinThread(this, eJM_Join);
}

///////////////////////////////////////////////////////////////////////////
bool CMainThread::IsActive()
{
	// JoinThread returns true if thread is not running.
	// JoinThread returns false if thread is still running
	return !gEnv->pThreadManager->JoinThread(this, eJM_TryJoin);
}

} // namespace CryAudio
