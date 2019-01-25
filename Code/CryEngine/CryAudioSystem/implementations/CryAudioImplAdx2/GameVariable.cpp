// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GameVariable.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Common.h"
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

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
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	float const finalValue = m_multiplier * value + m_shift;
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name), static_cast<CriFloat32>(finalValue));
	g_gameVariableValues[m_name] = finalValue;
#else
	criAtomEx_SetGameVariableByName(static_cast<CriChar8 const*>(m_name), static_cast<CriFloat32>(m_multiplier * value + m_shift));
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CGameVariable::SetGlobally(float const value)
{
	Set(nullptr, value);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
