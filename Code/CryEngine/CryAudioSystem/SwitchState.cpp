// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "SwitchState.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/ISwitchStateConnection.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Object.h"
	#include "Common/Logger.h"
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CSwitchState::~CSwitchState()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructSwitchStateConnection(pConnection);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::Set(CObject const& object) const
{
	for (auto const pConnection : m_connections)
	{
		object.GetImplDataPtr()->SetSwitchState(pConnection);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Log the "no-connections" case only on user generated controls.
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(SwitchState "%s" set on object "%s" without connections)", GetName(), object.m_name.c_str());
	}

	const_cast<CObject&>(object).StoreSwitchValue(m_switchId, m_switchStateId);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::SetGlobal() const
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->SetGlobalSwitchState(pConnection);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Log the "no-connections" case only on user generated controls.
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(SwitchState "%s" set globally without connections)", GetName());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}
} // namespace CryAudio
