// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Helpers/NativeEntityBase.h"

class CAudioAreaRandom final : public CNativeEntityBase
{

public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Enabled = 0,
		eProperty_PlayTrigger,
		eProperty_StopTrigger,
		eProperty_RTPC,
		eProperty_MoveWithEntity,
		eProperty_TriggerAreasOnMove,
		eProperty_SoundObstructionType,
		eProperty_RTPCDistance,
		eProperty_RadiusRandom,
		eProperty_MinDelay,
		eProperty_MaxDelay,
		eNumProperties
	};

public:
	CAudioAreaRandom() = default;
	virtual ~CAudioAreaRandom() {}

	// CNativeEntityBase
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

private:
	void       Reset();
	void       Play();
	void       Stop();
	Vec3       GenerateOffset();
	void       UpdateFadeValue(float distance);
	void       UpdateRtpc();
	const bool IsPlaying() const { return m_currentlyPlayingTriggerId != INVALID_AUDIO_CONTROL_ID; }

	AudioControlId m_playTriggerId = INVALID_AUDIO_CONTROL_ID;
	AudioControlId m_stopTriggerId = INVALID_AUDIO_CONTROL_ID;
	AudioControlId m_rtpcId = INVALID_AUDIO_CONTROL_ID;

	AudioControlId m_currentlyPlayingTriggerId = INVALID_AUDIO_CONTROL_ID;

	float          m_minDelay = 0.0f;
	float          m_maxDelay = 1000.0f;
	float          m_radius = 10.0f;
	float          m_fadeValue = 0.0;
	bool           m_bEnabled = true;
	EAreaState     m_areaState = EAreaState::Outside;
	float          m_rtpcDistance = 5.0f;

};
