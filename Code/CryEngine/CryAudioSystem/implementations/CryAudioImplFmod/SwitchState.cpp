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
CSwitchState::CSwitchState(
	uint32 const id,
	float const value,
	char const* const szName,
	EStateType const type)
	: m_id(id)
	, m_value(value)
	, m_name(szName)
	, m_type(type)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio