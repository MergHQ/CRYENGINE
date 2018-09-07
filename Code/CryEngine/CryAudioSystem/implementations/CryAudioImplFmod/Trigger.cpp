// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
TriggerToParameterIndexes g_triggerToParameterIndexes;

//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(
	uint32 const id,
	EEventType const eventType,
	FMOD::Studio::EventDescription* const pEventDescription,
	FMOD_GUID const guid,
	bool const hasProgrammerSound /*= false*/,
	char const* const szKey /*= ""*/)
	: m_id(id)
	, m_eventType(eventType)
	, m_pEventDescription(pEventDescription)
	, m_guid(guid)
	, m_hasProgrammerSound(hasProgrammerSound)
	, m_key(szKey)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
