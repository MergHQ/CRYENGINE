// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VcaParameter.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CVcaParameter::CVcaParameter(
	uint32 const id,
	float const multiplier,
	float const shift,
	char const* const szName,
	FMOD::Studio::VCA* const vca)
	: CParameter(id, multiplier, shift, szName, EParameterType::VCA)
	, m_vca(vca)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio