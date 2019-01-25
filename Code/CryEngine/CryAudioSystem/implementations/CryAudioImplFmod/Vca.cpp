// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Vca.h"

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include "Common.h"
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CVca::Set(IObject* const pIObject, float const value)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	float const finalValue = m_multiplier * value + m_shift;
	m_pVca->setVolume(finalValue);
	g_vcaValues[m_name] = finalValue;
#else
	m_pVca->setVolume(m_multiplier * value + m_shift);
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CVca::SetGlobally(float const value)
{
	Set(nullptr, value);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio