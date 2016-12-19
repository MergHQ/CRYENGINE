// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioEntitiesUtils.h"

#include <CrySystem/ISystem.h>
#include <CrySerialization/Enum.h>

using namespace CryAudio;

CryAudio::ControlId AudioEntitiesUtils::m_obstructionOcclusionSwitch = CryAudio::InvalidControlId;

YASLI_ENUM_BEGIN(EOcclusionType, "SoundObstructionType")
YASLI_ENUM_VALUE(eOcclusionType_Ignore, "Ignore")
YASLI_ENUM_VALUE(eOcclusionType_Adaptive, "Adaptive")
YASLI_ENUM_VALUE(eOcclusionType_Low, "Low")
YASLI_ENUM_VALUE(eOcclusionType_Medium, "Medium")
YASLI_ENUM_VALUE(eOcclusionType_High, "High")
YASLI_ENUM_END()

YASLI_ENUM_BEGIN(EPlayBehavior, "PlayBehavior")
YASLI_ENUM_VALUE(ePlayBehavior_Single, "Single")
YASLI_ENUM_VALUE(ePlayBehavior_Delay, "Delay")
YASLI_ENUM_VALUE(ePlayBehavior_TriggerRate, "TriggerRate")
YASLI_ENUM_END()

CryAudio::ControlId AudioEntitiesUtils::GetObstructionOcclusionSwitch()
{
	if (m_obstructionOcclusionSwitch == InvalidControlId)
	{
		Init();
	}
	return m_obstructionOcclusionSwitch;
}

const std::array<ControlId, numOcclusionTypes>& AudioEntitiesUtils::GetObstructionOcclusionStateIds()
{
	if (m_obstructionOcclusionSwitch == InvalidControlId)
	{
		Init();
	}
	return m_obstructionStateIds;
}

const std::array<const char*, numOcclusionTypes>& AudioEntitiesUtils::GetObstructionNames()
{
	return m_obstructionNames;
}

void AudioEntitiesUtils::Init()
{
	// Update the switch ID
	gEnv->pAudioSystem->GetAudioSwitchId("ObstrOcclCalcType", m_obstructionOcclusionSwitch);

	// Update the state IDs
	for (int i = 0; i < numOcclusionTypes; ++i)
	{
		gEnv->pAudioSystem->GetAudioSwitchStateId(m_obstructionOcclusionSwitch, m_obstructionNames[i], m_obstructionStateIds[i]);
	}
}

std::array<const char*, numOcclusionTypes> AudioEntitiesUtils::m_obstructionNames = {
	{ "ignore", "adaptive", "low", "medium", "high" }
};

std::array<ControlId, numOcclusionTypes> AudioEntitiesUtils::m_obstructionStateIds = {
	{ 0 }
};

float AudioEntitiesUtils::AreaFadeEpsilon = 0.025f;
