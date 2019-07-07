// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <array>
#include <CryAudio/IAudioSystem.h>

enum EPlayBehavior
{
	ePlayBehavior_Single = 0,
	ePlayBehavior_Delay,
	ePlayBehavior_TriggerRate,
};

enum class EAreaState
{
	Outside = 0,
	Near,
	Inside
};

class AudioEntitiesUtils
{
	AudioEntitiesUtils() = delete;

public:

	static float AreaFadeEpsilon;
};
