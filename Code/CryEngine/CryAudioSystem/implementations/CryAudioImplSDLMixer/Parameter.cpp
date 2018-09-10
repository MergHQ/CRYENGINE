// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
CParameter::CParameter(
	SampleId const sampleId,
	float const multiplier,
	float const shift,
	char const* const szName)
	: m_sampleId(sampleId)
	, m_multiplier(multiplier)
	, m_shift(shift)
	, m_name(szName)
{
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
