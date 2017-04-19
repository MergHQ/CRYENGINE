// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioEntitiesUtils.h"

#include <CrySystem/ISystem.h>
#include <CrySerialization/Enum.h>

using namespace CryAudio;

CryAudio::ControlId AudioEntitiesUtils::m_obstructionOcclusionSwitch = CryAudio::InvalidControlId;

SERIALIZATION_ENUM_BEGIN_NESTED(CryAudio, EOcclusionType, "OcclusionType")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Ignore, "ignore_state_name", "Ignore");
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Adaptive, "adaptive_state_name", "Adaptive");
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Low, "low_state_name", "Low");
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Medium, "medium_state_name", "Medium");
SERIALIZATION_ENUM(CryAudio::EOcclusionType::High, "high_state_name", "High");
SERIALIZATION_ENUM_END();

SERIALIZATION_ENUM_BEGIN(EPlayBehavior, "PlayBehavior")
SERIALIZATION_ENUM(ePlayBehavior_Single, "single_state_name", "Single")
SERIALIZATION_ENUM(ePlayBehavior_Delay, "delay_state_name", "Delay")
SERIALIZATION_ENUM(ePlayBehavior_TriggerRate, "trigger_rate_state_name", "TriggerRate")
SERIALIZATION_ENUM_END()

ControlId AudioEntitiesUtils::GetObstructionOcclusionSwitch()
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
	{ IgnoreStateName, AdaptiveStateName, LowStateName, MediumStateName, HighStateName }
};

std::array<ControlId, numOcclusionTypes> AudioEntitiesUtils::m_obstructionStateIds = {
	{ 0 }
};

float AudioEntitiesUtils::AreaFadeEpsilon = 0.025f;
