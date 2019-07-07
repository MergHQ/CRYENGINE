// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioEntitiesUtils.h"

#include <CrySystem/ISystem.h>
#include <CrySerialization/Enum.h>

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

float AudioEntitiesUtils::AreaFadeEpsilon = 0.025f;
