// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryThreading/IThreadManager.h>
#include <CryThreading/CryThread.h>

namespace CryAudio
{
class CMainThread final : public ::IThread
{
public:

	CMainThread() = default;
	CMainThread(CMainThread const&) = delete;
	CMainThread(CMainThread&&) = delete;
	CMainThread& operator=(CMainThread const&) = delete;
	CMainThread& operator=(CMainThread&&) = delete;

	// IThread
	virtual void ThreadEntry() override;
	// ~IThread

	// Signals the thread that it should not accept anymore work and exit
	void SignalStopWork();

	bool IsActive();
	void Activate();
	void Deactivate();

private:

	volatile bool m_doWork = true;
};
} // namespace CryAudio
