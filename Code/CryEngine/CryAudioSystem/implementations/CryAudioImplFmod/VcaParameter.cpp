// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaParameter.h"
#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CVcaParameter::Set(IObject* const pIObject, float const value)
{
	FMOD_RESULT const fmodResult = m_pVca->setVolume(m_multiplier * value + m_shift);
	ASSERT_FMOD_OK;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio