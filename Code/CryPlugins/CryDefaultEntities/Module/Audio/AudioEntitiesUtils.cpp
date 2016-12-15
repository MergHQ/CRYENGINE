// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioEntitiesUtils.h"

#include <CrySystem/ISystem.h>
#include <CrySerialization/Enum.h>

YASLI_ENUM_BEGIN(ESoundObstructionType, "SoundObstructionType")
YASLI_ENUM_VALUE(eSoundObstructionType_Ignore, "Ignore")
YASLI_ENUM_VALUE(eSoundObstructionType_Adaptive, "Adaptive")
YASLI_ENUM_VALUE(eSoundObstructionType_Low, "Low")
YASLI_ENUM_VALUE(eSoundObstructionType_Medium, "Medium")
YASLI_ENUM_VALUE(eSoundObstructionType_High, "High")
YASLI_ENUM_END()

YASLI_ENUM_BEGIN(EPlayBehavior, "PlayBehavior")
YASLI_ENUM_VALUE(ePlayBehavior_Single, "Single")
YASLI_ENUM_VALUE(ePlayBehavior_Delay, "Delay")
YASLI_ENUM_VALUE(ePlayBehavior_TriggerRate, "TriggerRate")
YASLI_ENUM_END()

AudioControlId AudioEntitiesUtils::GetObstructionOcclusionSwitch()
{
	if (m_obstructionOcclusionSwitch == INVALID_AUDIO_CONTROL_ID)
	{
		Init();
	}
	return m_obstructionOcclusionSwitch;
}

const std::array<AudioControlId, 5>& AudioEntitiesUtils::GetObstructionOcclusionStateIds()
{
	if (m_obstructionOcclusionSwitch == INVALID_AUDIO_CONTROL_ID)
	{
		Init();
	}
	return m_obstructionStateIds;
}

const std::array<const char*, 5>& AudioEntitiesUtils::GetObstructionNames()
{
	return m_obstructionNames;
}

void AudioEntitiesUtils::Init()
{
	// Update the switch ID
	gEnv->pAudioSystem->GetAudioSwitchId("ObstrOcclCalcType", m_obstructionOcclusionSwitch);

	// Update the state IDs
	for (int i = 0; i < eSoundObstructionType_Num; ++i)
	{
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_obstructionOcclusionSwitch, m_obstructionNames[i], m_obstructionStateIds[i]);
	}
}

AudioControlId AudioEntitiesUtils::m_obstructionOcclusionSwitch = INVALID_AUDIO_CONTROL_ID;
std::array<const char*, eSoundObstructionType_Num> AudioEntitiesUtils::m_obstructionNames = {
	{ "ignore", "adaptive", "low", "medium", "high" }
};

std::array<AudioControlId, eSoundObstructionType_Num> AudioEntitiesUtils::m_obstructionStateIds = {
	{ 0 }
};

float AudioEntitiesUtils::AreaFadeEpsilon = 0.025f;
