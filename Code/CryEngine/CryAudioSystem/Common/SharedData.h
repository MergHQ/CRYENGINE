// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

/**
 * @namespace CryAudio
 * @brief Most parent audio namespace used throughout the entire engine.
 */
namespace CryAudio
{
enum class EEventState : EnumFlagsType
{
	None,
	Playing,
	Loading,
	Unloading,
	Virtual,
};
} // namespace CryAudio
