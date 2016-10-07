// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Helpers/NativeEntityBase.h"
#include "AudioEntitiesUtils.h"

class CAudioAreaAmbienceEntity : public CNativeEntityBase
{
public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Active = 0,
		eProperty_PlayTrigger,
		eProperty_StopTrigger,
		eProperty_TriggerAreasOnMove,

		eProperty_Rtpc,
		eProperty_GlobalRtpc,
		eProperty_RtpcDistance,

		eProperty_Environment,
		eProperty_EnvironmentDistance,

		eProperty_SoundObstructionType,

		eNumProperties
	};

public:
	CAudioAreaAmbienceEntity() = default;
	virtual ~CAudioAreaAmbienceEntity() {}

	// CNativeEntityBase
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

protected:
	// Called on entity spawn, or when the state of the entity changes in Editor
	void       Reset();

	void       Play(AudioControlId triggerId);
	void       Stop();

	void       UpdateRtpc(float fadeValue);
	void       UpdateFadeValue(float distance);

	const bool IsPlaying() const { return m_playingTriggerId != INVALID_AUDIO_CONTROL_ID; }

	void       SetObstruction();
	void       DisableObstruction();

protected:
	AudioControlId m_playTriggerId = INVALID_AUDIO_CONTROL_ID;
	AudioControlId m_stopTriggerId = INVALID_AUDIO_CONTROL_ID;

	// Id of the trigger we are currently executing
	AudioControlId m_playingTriggerId = INVALID_AUDIO_CONTROL_ID;

	AudioControlId m_rtpcId = INVALID_AUDIO_CONTROL_ID;
	AudioControlId m_globalRtpcId = INVALID_AUDIO_CONTROL_ID;

	AudioControlId m_environmentId = INVALID_AUDIO_CONTROL_ID;

	AudioControlId m_obstructionSwitchId = INVALID_AUDIO_CONTROL_ID;

	EAreaState     m_areaState = EAreaState::Outside;
	float          m_fadeValue = 0.0f;
};
