// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ParameterAdvanced.h"
#include "Common.h"
#include "Object.h"

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
CParameterAdvanced::~CParameterAdvanced()
{
	for (auto const pObject : g_constructedObjects)
	{
		pObject->RemoveParameter(m_parameterInfo);
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterAdvanced::Set(IObject* const pIObject, float const value)
{
	if (!m_parameterInfo.IsGlobal())
	{
		auto const pObject = static_cast<CObject*>(pIObject);
		pObject->SetParameter(m_parameterInfo, m_multiplier * value + m_shift);
	}
	else
	{
		g_pStudioSystem->setParameterByID(m_parameterInfo.GetId(), m_multiplier * value + m_shift);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
		auto const pObject = static_cast<CObject*>(pIObject);
		float const finalValue = m_multiplier * value + m_shift;
		Cry::Audio::Log(ELogType::Warning,
		                R"(FMOD - Global parameter "%s" was set to %f on object "%s". Consider setting it globally.)",
		                m_parameterInfo.GetName(),
		                finalValue,
		                pObject->GetName());
#endif    // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CParameterAdvanced::SetGlobally(float const value)
{
	if (m_parameterInfo.IsGlobal())
	{
		g_pStudioSystem->setParameterByID(m_parameterInfo.GetId(), m_multiplier * value + m_shift);
	}
	else
	{
		float const finalValue = m_multiplier * value + m_shift;

		for (auto const pObject : g_constructedObjects)
		{
			pObject->SetParameter(m_parameterInfo, finalValue);
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