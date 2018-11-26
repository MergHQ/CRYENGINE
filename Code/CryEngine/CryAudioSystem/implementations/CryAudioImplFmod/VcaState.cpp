// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaState.h"
#include "Common.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
void CVcaState::Set(IObject* const pIObject)
{
	FMOD_RESULT const fmodResult = m_pVca->setVolume(m_value);
	ASSERT_FMOD_OK;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
