// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include "Common_fmod.h"

// Fmod forward declarations.
namespace FMOD
{
class ChannelGroup;
class DSP;

namespace Studio
{
class EventInstance;
class ParameterInstance;
}
}

namespace CryAudio
{
namespace Impl
{
class CAudioEnvironment_fmod;
class CAudioObject_fmod;

class CAudioEvent_fmod final : public IAudioEvent
{
public:

	explicit CAudioEvent_fmod(AudioEventId const _id)
		: m_id(_id)
		, m_eventPathId(AUDIO_INVALID_CRC32)
		, m_lowpassFrequencyMax(0.0f)
		, m_lowpassFrequencyMin(0.0f)
		, m_pInstance(nullptr)
		, m_pMasterTrack(nullptr)
		, m_pLowpass(nullptr)
		, m_pOcclusionParameter(nullptr)
		, m_pAudioObject(nullptr)
	{}

	virtual ~CAudioEvent_fmod() {}

	bool                         PrepareForOcclusion();
	void                         SetObstructionOcclusion(float const obstruction, float const occlusion);
	void                         Reset();
	AudioEventId                 GetId() const                                             { return m_id; }
	uint32                       GetEventPathId() const                                    { return m_eventPathId; }
	void                         SetEventPathId(uint32 const eventPathId)                  { m_eventPathId = eventPathId; }
	FMOD::Studio::EventInstance* GetInstance() const                                       { return m_pInstance; }
	void                         SetInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }
	CAudioObject_fmod*           GetAudioObjectData() const                                { return m_pAudioObject; }
	void                         SetAudioObjectData(CAudioObject_fmod* const pAudioObject) { m_pAudioObject = pAudioObject; }
	void                         TrySetEnvironment(CAudioEnvironment_fmod const* const pEnvironment, float const value);

private:

	AudioEventId const               m_id;
	uint32                           m_eventPathId;

	float                            m_lowpassFrequencyMax;
	float                            m_lowpassFrequencyMin;

	FMOD::Studio::EventInstance*     m_pInstance;
	FMOD::ChannelGroup*              m_pMasterTrack;
	FMOD::DSP*                       m_pLowpass;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter;
	CAudioObject_fmod*               m_pAudioObject;

	DELETE_DEFAULT_CONSTRUCTOR(CAudioEvent_fmod);
	PREVENT_OBJECT_COPY(CAudioEvent_fmod);
};

}
}
