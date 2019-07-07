// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CryThreading/Synchronization.h>
#include <CryThreading/IThreadManager.h>
#include <CryThreading/IThreadConfigManager.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CAuxThread final : public IThread
{
public:

	CAuxThread(CAuxThread const&) = delete;
	CAuxThread(CAuxThread&&) = delete;
	CAuxThread& operator=(CAuxThread const&) = delete;
	CAuxThread& operator=(CAuxThread&&) = delete;

	enum class EAuxThreadState : EnumFlagsType
	{
		Wait,
		Start,
		Stop,
	};

	CAuxThread()
		: m_threadState(EAuxThreadState::Wait)
		, m_bQuit(false)
	{}

	virtual ~CAuxThread() override = default;

	// IThread
	virtual void ThreadEntry() override;
	// ~IThread

	void Init();
	void SignalStopWork();

	EAuxThreadState      m_threadState;
	CryMutex             m_lock;
	CryConditionVariable m_sem;

private:

	volatile bool m_bQuit;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
