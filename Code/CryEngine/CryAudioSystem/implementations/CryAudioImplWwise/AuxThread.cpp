// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AuxThread.h"
#include "Common.h"
#include "Impl.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CAuxThread::Init()
{
	if (!gEnv->pThreadManager->SpawnThread(this, "AuxWwiseAudioThread"))
	{
		CryFatalError(R"(Error spawning "AuxWwiseAudioThread" thread.)");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAuxThread::ThreadEntry()
{
	while (!m_bQuit)
	{
		m_lock.Lock();

		if (m_threadState == EAuxThreadState::Stop)
		{
			m_threadState = EAuxThreadState::Wait;
			m_sem.Notify();
		}

		while (m_threadState == EAuxThreadState::Wait)
		{
			m_sem.Wait(m_lock);
		}

		m_lock.Unlock();

		if (m_bQuit)
		{
			break;
		}

		g_pImpl->Update();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAuxThread::SignalStopWork()
{
	m_bQuit = true;
	m_threadState = EAuxThreadState::Start;
	m_sem.Notify();
	gEnv->pThreadManager->JoinThread(this, eJM_Join);
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
