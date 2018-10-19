// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaState.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CVcaState::CVcaState(
	uint32 const id,
	float const value,
	FMOD::Studio::VCA* const vca)
	: CBaseSwitchState(id, value, EStateType::VCA)
	, m_vca(vca)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio