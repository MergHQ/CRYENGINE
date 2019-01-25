// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaState.h"

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
void CVcaState::Set(IObject* const pIObject)
{
	m_pVca->setVolume(m_value);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	g_vcaValues[m_name] = m_value;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CVcaState::SetGlobally()
{
	Set(nullptr);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
