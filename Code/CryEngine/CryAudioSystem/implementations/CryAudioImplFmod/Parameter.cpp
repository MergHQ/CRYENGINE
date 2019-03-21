// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "Common.h"
#include "BaseObject.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CParameter::~CParameter()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->RemoveParameter(m_parameterInfo);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::Set(IObject* const pIObject, float const value)
{
	if (!m_parameterInfo.IsGlobal())
	{
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
		pBaseObject->SetParameter(m_parameterInfo, m_multiplier * value + m_shift);
	}
	else
	{
		g_pStudioSystem->setParameterByID(m_parameterInfo.GetId(), m_multiplier * value + m_shift);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
		auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
		float const finalValue = m_multiplier * value + m_shift;
		Cry::Audio::Log(ELogType::Warning,
		                R"(FMOD - Global parameter "%s" was set to %f on object "%s". Consider setting it globally.)",
		                m_parameterInfo.GetName(),
		                finalValue,
		                pBaseObject->GetName());
#endif    // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value)
{
	if (m_parameterInfo.IsGlobal())
	{
		g_pStudioSystem->setParameterByID(m_parameterInfo.GetId(), m_multiplier * value + m_shift);
	}
	else
	{
		float const finalValue = m_multiplier * value + m_shift;

		for (auto const pBaseObject : g_constructedObjects)
		{
			// pBaseObject->SetParameter(m_id, finalValue);
			pBaseObject->SetParameter(m_parameterInfo, finalValue);
		}

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
		Cry::Audio::Log(ELogType::Warning,
		                R"(FMOD - Local parameter "%s" was set globally to %f on all objects. Consider using a global parameter for better performance.)",
		                m_parameterInfo.GetName(),
		                finalValue);
#endif    // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio