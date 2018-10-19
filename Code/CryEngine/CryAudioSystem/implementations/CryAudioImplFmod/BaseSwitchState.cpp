// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseSwitchState.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseSwitchState::CBaseSwitchState(
	uint32 const id,
	float const value,
	EStateType const type)
	: m_id(id)
	, m_value(value)
	, m_type(type)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio