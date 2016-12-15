// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Helpers/DesignerEntityComponent.h"
#include "AudioEntitiesUtils.h"

#include <CrySerialization/Decorators/ResourcesAudio.h>

class CAudioAreaAmbienceEntity final 
	: public CDesignerEntityComponent<>
	, public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CAudioAreaAmbienceEntity, "AudioAreaAmbience", 0x66F9FDBA753B4A3C, 0x977230560C65881B);

	virtual ~CAudioAreaAmbienceEntity() {}

public:
	// CDesignerEntityComponent
	virtual void ProcessEvent(SEntityEvent& event) override;
	virtual uint64 GetEventMask() const override { return CDesignerEntityComponent::GetEventMask() | BIT64(ENTITY_EVENT_ENTERNEARAREA) | BIT64(ENTITY_EVENT_MOVENEARAREA) | BIT64(ENTITY_EVENT_ENTERAREA) | BIT64(ENTITY_EVENT_MOVEINSIDEAREA) | BIT64(ENTITY_EVENT_LEAVEAREA) | BIT64(ENTITY_EVENT_LEAVENEARAREA); }

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void OnResetState() override;
	// ~CDesignerEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "AudioAreaAmbience Properties"; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override
	{
		archive(m_bEnabled, "Enabled", "Enabled");
		archive(Serialization::AudioTrigger(m_playTriggerName), "PlayTrigger", "PlayTrigger");
		archive(Serialization::AudioTrigger(m_stopTriggerName), "StopTrigger", "StopTrigger");
		archive(m_bTriggerAreasOnMove, "TriggerAreasOnMove", "TriggerAreasOnMove");

		archive(Serialization::AudioRTPC(m_rtpcName), "Rtpc", "Rtpc");
		archive(Serialization::AudioRTPC(m_globalRtpcName), "GlobalRtpc", "GlobalRtpc");
		archive(m_rtpcDistance, "RtpcDistance", "RtpcDistance");

		archive(Serialization::AudioEnvironment(m_environmentName), "Environment", "Environment");
		archive(m_environmentDistance, "EnvironmentDistance", "EnvironmentDistance");
		
		archive(m_obstructionType, "SoundObstructionType", "SoundObstructionType");

		if (archive.isInput())
		{
			OnResetState();
		}
	}
	// ~IEntityPropertyGroup

protected:
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

protected:
	bool m_bEnabled = true;
	bool m_bTriggerAreasOnMove = false;

	string m_playTriggerName;
	string m_stopTriggerName;

	string m_rtpcName;
	string m_globalRtpcName;

	float m_rtpcDistance = 5.f;

	string m_environmentName;
	float m_environmentDistance = 5.f;
	
	ESoundObstructionType m_obstructionType = eSoundObstructionType_Adaptive;
};