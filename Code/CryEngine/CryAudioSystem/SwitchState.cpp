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
	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
		for (auto const pConnection : m_connections)
		{
			pConnection->Set(pIObject);
		}
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}

	// Log the "no-connections" case only on user generated controls.
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(SwitchState "%s" set on object "%s" without connections)", GetName(), object.GetName());
	}

	const_cast<CObject&>(object).StoreSwitchValue(m_switchId, m_switchStateId);
#endif   // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSwitchState::SetGlobally() const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->SetGlobally();
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
