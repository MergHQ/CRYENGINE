// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "EnvironmentBus.h"

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CEnvironmentBus::CEnvironmentBus(
	FMOD::Studio::EventDescription* const pEventDescription,
	FMOD::Studio::Bus* const pBus)
	: CEnvironment(EEnvironmentType::Bus)
	, m_pEventDescription(pEventDescription)
	, m_pBus(pBus)
{
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
