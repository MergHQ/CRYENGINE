// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
//////////////////////////////////////////////////////////////////////////
CSwitchState::CSwitchState(
	SampleId const sampleId,
	float const value,
	char const* const szName)
	: m_sampleId(sampleId)
	, m_value(value)
	, m_name(szName)
{
}
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio