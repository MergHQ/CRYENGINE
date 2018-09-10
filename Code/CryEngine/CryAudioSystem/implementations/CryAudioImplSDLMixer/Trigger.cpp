// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
CTrigger::CTrigger(
	EEventType const type,
	SampleId const sampleId,
	float const attenuationMinDistance /*= s_defaultMinAttenuationDist*/,
	float const attenuationMaxDistance /*= s_defaultMaxAttenuationDist*/,
	int const volume /*= 128*/,
	int const numLoops /*= 1*/,
	int const fadeInTime /*= 0*/,
	int const fadeOutTime /*= 0*/,
	bool const isPanningEnabled /*= true*/)
	: m_type(type)
	, m_sampleId(sampleId)
	, m_attenuationMinDistance(attenuationMinDistance)
	, m_attenuationMaxDistance(attenuationMaxDistance)
	, m_volume(volume)
	, m_numLoops(numLoops)
	, m_fadeInTime(fadeInTime)
	, m_fadeOutTime(fadeOutTime)
	, m_isPanningEnabled(isPanningEnabled)
{
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
