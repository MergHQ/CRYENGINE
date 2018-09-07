// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CParameter::CParameter(
	uint32 const id,
	float const multiplier,
	float const shift,
	char const* const szName,
	EParameterType const type)
	: m_id(id)
	, m_multiplier(multiplier)
	, m_shift(shift)
	, m_name(szName)
	, m_type(type)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
