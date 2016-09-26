// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

struct SQueuedAudioCommandBase;

class CAudioProxy final : public IAudioProxy
{
public:

	CAudioProxy() = default;
	virtual ~CAudioProxy() override;
	CAudioProxy(CAudioProxy const&) = delete;
	CAudioProxy(CAudioProxy&&) = delete;
	CAudioProxy& operator=(CAudioProxy const&) = delete;
	CAudioProxy& operator=(CAudioProxy&&) = delete;

	// IAudioProxy
	virtual void          Initialize(char const* const szAudioObjectName, bool const bInitAsync = true) override;
	virtual void          Release() override;
	virtual void          Reset() override;
	virtual void          PlayFile(SAudioPlayFileInfo const& playFileInfo, SAudioCallBackInfo const& callBackInfo = SAudioCallBackInfo::GetEmptyObject()) override;
	virtual void          StopFile(char const* const szFile) override;
	virtual void          ExecuteTrigger(AudioControlId const audioTriggerId, SAudioCallBackInfo const& callBackInfo = SAudioCallBackInfo::GetEmptyObject()) override;
	virtual void          StopTrigger(AudioControlId const audioTriggerId) override;
	virtual void          SetSwitchState(AudioControlId const audioSwitchId, AudioSwitchStateId const audioSwitchStateId) override;
	virtual void          SetRtpcValue(AudioControlId const audioRtpcId, float const value) override;
	virtual void          SetOcclusionType(EAudioOcclusionType const occlusionType) override;
	virtual void          SetTransformation(Matrix34 const& transformation) override;
	virtual void          SetPosition(Vec3 const& position) override;
	virtual void          SetEnvironmentAmount(AudioEnvironmentId const audioEnvironmentId, float const amount) override;
	virtual void          SetCurrentEnvironments(EntityId const entityToIgnore = 0) override;
	virtual AudioObjectId GetAudioObjectId() const override { return m_audioObjectId; }
	// ~IAudioProxy

	void ClearEnvironments();
	void ExecuteQueuedCommands();

	static AudioControlId     s_occlusionTypeSwitchId;
	static AudioSwitchStateId s_occlusionTypeStateIds[eAudioOcclusionType_Count];

private:

	enum EAudioProxyFlags : AudioEnumFlagsType
	{
		eAudioProxyFlags_None         = 0,
		eAudioProxyFlags_WaitingForId = BIT(0),
	};

	typedef std::deque<SQueuedAudioCommandBase const*, STLSoundAllocator<SQueuedAudioCommandBase const*>> TQueuedAudioCommands;
	TQueuedAudioCommands m_queuedAudioCommands;

	void QueueCommand(SQueuedAudioCommandBase const* const pNewCommandBase);
	void SetTransformationInternal(Matrix34 const& transformation);

	AudioObjectId              m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
	AudioEnumFlagsType         m_flags = eAudioProxyFlags_None;
	CAudioObjectTransformation m_transformation;
};
