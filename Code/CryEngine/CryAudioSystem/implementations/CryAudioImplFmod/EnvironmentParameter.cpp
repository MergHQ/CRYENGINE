// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EnvironmentParameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CEnvironmentParameter::CEnvironmentParameter(
	uint32 const id,
	float const multiplier,
	float const shift,
	char const* const szName)
	: CEnvironment(EEnvironmentType::Parameter)
	, m_id(id)
	, m_multiplier(multiplier)
	, m_shift(shift)
	, m_name(szName)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
