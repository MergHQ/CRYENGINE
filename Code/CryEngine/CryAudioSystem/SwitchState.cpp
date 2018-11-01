// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"
#include "SwitchStateConnection.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Object.h"
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSwitchState::~CSwitchState()
{
	for (auto const pStateImpl : m_connections)
	{
		delete pStateImpl;
	}
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::Set(CObject const& object) const
{
	for (auto const pSwitchStateImpl : m_connections)
	{
		pSwitchStateImpl->Set(object);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	const_cast<CObject&>(object).StoreSwitchValue(m_switchId, m_switchStateId);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}
} // namespace CryAudio
