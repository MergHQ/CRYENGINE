// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Helpers/NativeEntityBase.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

class CAudioTriggerSpotEntity final : public CNativeEntityBase
{

public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Enabled = 0,
		eProperty_PlayTrigger,
		eProperty_StopTrigger,
		eProperty_TriggerAreasOnMove,
		eProperty_SoundObstructionType,
		eProperty_Behaviour,
		eProperty_MinDelay,
		eProperty_MaxDelay,
		eProperty_RandomizationArea,
		eProperty_DrawActivityRadius,
		eProperty_DrawRandomizationArea,
		eNumProperties
	};

	enum EPlayBehaviour
	{
		ePlayBehaviour_Single = 0,
		ePlayBehaviour_Delay,
		ePlayBehaviour_TriggerRate,
	};

public:
	CAudioTriggerSpotEntity();
	virtual ~CAudioTriggerSpotEntity();

	// CNativeEntityBase
	virtual void PostInit(IGameObject* pGameObject) override;
	virtual void ProcessEvent(SEntityEvent& event) override;

#if !defined(_RELEASE)
	virtual void Update(SEntityUpdateContext& ctx, int updateSlot) override;
#endif

	virtual void OnResetState() override;
	// ~CNativeEntityBase

private:
	void        Stop();
	void        Play();
	void        StartPlayingBehaviour();
	Vec3        GenerateOffset();
	static void OnAudioTriggerFinished(SAudioRequestInfo const* const pAudioRequestInfo);
	void        TriggerFinished(const AudioControlId trigger);

	AudioControlId m_playTriggerId = INVALID_AUDIO_CONTROL_ID;
	AudioControlId m_stopTriggerId = INVALID_AUDIO_CONTROL_ID;

	AudioControlId m_currentlyPlayingTriggerId = INVALID_AUDIO_CONTROL_ID;
	Vec3           m_randomizationArea;
	bool           m_bEnabled = true;
	EPlayBehaviour m_behaviour = ePlayBehaviour_Single;
	float          m_minDelay = 0.0f;
	float          m_maxDelay = 1000.0f;

#if !defined(_RELEASE)
	enum EDrawActivityRadius
	{
		eDrawActivityRadius_Disabled = 0,
		eDrawActivityRadius_PlayTrigger,
		eDrawActivityRadius_StopTrigger,
	};
	EDrawActivityRadius m_drawActivityRadius = eDrawActivityRadius_Disabled;
	bool                m_bDrawRandomizationArea = false;
#endif
};
