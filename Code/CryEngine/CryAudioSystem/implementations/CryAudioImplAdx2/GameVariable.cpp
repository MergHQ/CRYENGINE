// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GameVariable.h"

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CGameVariable::Set(IObject* const pIObject, float const value)
{
	SetGlobally(value);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	float const finalValue = m_multiplier * value + m_shift;
	Cry::Audio::Log(ELogType::Warning, R"(Adx2 - GameVariable "%s" was set to %f on object "%s". Consider setting it globally.)",
	                m_name.c_str(), finalValue, pBaseObject->GetName());
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGameVariable::SetGlobally(float const value)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	float const finalValue = m_multiplier * value + m_shift;
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name.c_str()), static_cast<CriFloat32>(finalValue));
	g_gameVariableValues[m_name] = finalValue;
#else
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name.c_str()), static_cast<CriFloat32>(m_multiplier * value + m_shift));
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
