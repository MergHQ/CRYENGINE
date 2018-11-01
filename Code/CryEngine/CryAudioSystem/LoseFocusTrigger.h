// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Entity.h"
#include "Common.h"
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CLoseFocusTrigger final : public Control
{
public:

	CLoseFocusTrigger(CLoseFocusTrigger const&) = delete;
	CLoseFocusTrigger(CLoseFocusTrigger&&) = delete;
	CLoseFocusTrigger& operator=(CLoseFocusTrigger const&) = delete;
	CLoseFocusTrigger& operator=(CLoseFocusTrigger&&) = delete;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CLoseFocusTrigger()
		: Control(LoseFocusTriggerId, EDataScope::Global, s_szLoseFocusTriggerName)
	{}
#else
	CLoseFocusTrigger()
		: Control(LoseFocusTriggerId, EDataScope::Global)
	{}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	~CLoseFocusTrigger();

	void Execute() const;
	void AddConnections(TriggerConnections const& connections);
	void Clear();

private:

	TriggerConnections m_connections;
};
} // namespace CryAudio
