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
	float const shift)
	: CBaseParameter(id, multiplier, shift, EParameterType::Parameter)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
