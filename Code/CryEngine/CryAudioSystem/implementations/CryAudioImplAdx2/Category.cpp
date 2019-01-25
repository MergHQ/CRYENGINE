// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Category.h"

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
void CCategory::Set(IObject* const pIObject, float const value)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	float const finalValue = m_multiplier * value + m_shift;
	criAtomExCategory_SetVolumeByName(static_cast<CriChar8 const*>(m_name), static_cast<CriFloat32>(finalValue));
	g_categoryValues[m_name] = finalValue;
#else
	criAtomExCategory_SetVolumeByName(static_cast<CriChar8 const*>(m_name), static_cast<CriFloat32>(m_multiplier * value + m_shift));
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CCategory::SetGlobally(float const value)
{
	Set(nullptr, value);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
