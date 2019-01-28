// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Parameter.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/IParameterConnection.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Object.h"
	#include "Common/Logger.h"
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CParameter::~CParameter()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructParameterConnection(pConnection);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::Set(CObject const& object, float const value) const
{
	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
		for (auto const pConnection : m_connections)
		{
			pConnection->Set(pIObject, value);
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
		Cry::Audio::Log(ELogType::Warning, R"(Parameter "%s" set on object "%s" without connections)", GetName(), object.GetName());
	}

	const_cast<CObject&>(object).StoreParameterValue(GetId(), value);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value) const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->SetGlobally(value);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	// Log the "no-connections" case only on user generated controls.
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Parameter "%s" set globally without connections)", GetName());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}
} // namespace CryAudio
