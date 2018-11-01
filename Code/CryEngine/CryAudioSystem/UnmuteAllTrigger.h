// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CUnmuteAllTrigger final : public Control
{
public:

	CUnmuteAllTrigger(CUnmuteAllTrigger const&) = delete;
	CUnmuteAllTrigger(CUnmuteAllTrigger&&) = delete;
	CUnmuteAllTrigger& operator=(CUnmuteAllTrigger const&) = delete;
	CUnmuteAllTrigger& operator=(CUnmuteAllTrigger&&) = delete;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CUnmuteAllTrigger()
		: Control(UnmuteAllTriggerId, EDataScope::Global, s_szUnmuteAllTriggerName)
	{}
#else
	CUnmuteAllTrigger()
		: Control(UnmuteAllTriggerId, EDataScope::Global)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	~CUnmuteAllTrigger();

	void Execute() const;
	void AddConnections(TriggerConnections const& connections);
	void Clear();

private:

	TriggerConnections m_connections;
};
} // namespace CryAudio
