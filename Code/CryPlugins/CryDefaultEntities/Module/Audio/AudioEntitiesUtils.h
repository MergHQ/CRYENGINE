// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <array>
#include "CryAudio/IAudioInterfacesCommonData.h"
#include "CryAudio/IAudioSystem.h"

enum ESoundObstructionType
{
	eSoundObstructionType_Ignore = 0,
	eSoundObstructionType_Adaptive,
	eSoundObstructionType_Low,
	eSoundObstructionType_Medium,
	eSoundObstructionType_High,
	eSoundObstructionType_Num,
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

	static void                                                         Init();
	static AudioControlId                                               GetObstructionOcclusionSwitch();
	static const std::array<AudioControlId, eSoundObstructionType_Num>& GetObstructionOcclusionStateIds();
	static const std::array<const char*, eSoundObstructionType_Num>&    GetObstructionNames();

	static float AreaFadeEpsilon;

private:

	static AudioControlId m_obstructionOcclusionSwitch;
	static std::array<const char*, eSoundObstructionType_Num>    m_obstructionNames;
	static std::array<AudioControlId, eSoundObstructionType_Num> m_obstructionStateIds;

};
