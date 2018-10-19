// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CSwitchState::CSwitchState(uint32 const id, float const value)
	: CBaseSwitchState(id, value, EStateType::State)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio