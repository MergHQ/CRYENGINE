// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CResumeAllTrigger final : public Control
{
public:

	CResumeAllTrigger(CResumeAllTrigger const&) = delete;
	CResumeAllTrigger(CResumeAllTrigger&&) = delete;
	CResumeAllTrigger& operator=(CResumeAllTrigger const&) = delete;
	CResumeAllTrigger& operator=(CResumeAllTrigger&&) = delete;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CResumeAllTrigger()
		: Control(g_resumeAllTriggerId, EDataScope::Global, g_szResumeAllTriggerName)
	{}
#else
	CResumeAllTrigger()
		: Control(g_resumeAllTriggerId, EDataScope::Global)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	~CResumeAllTrigger();

	void Execute() const;
	void AddConnections(TriggerConnections const& connections);
	void Clear();

private:

	TriggerConnections m_connections;
};
} // namespace CryAudio
