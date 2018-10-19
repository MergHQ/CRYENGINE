// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseParameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseParameter::CBaseParameter(
	uint32 const id,
	float const multiplier,
	float const shift,
	EParameterType const type)
	: m_id(id)
	, m_multiplier(multiplier)
	, m_shift(shift)
	, m_type(type)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
