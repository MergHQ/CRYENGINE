// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>
#include <PoolObject.h>

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
namespace Fmod
{
class CAudioEnvironment;
class CAudioObjectBase;

class CAudioEvent final : public IAudioEvent, public CPoolObject<CAudioEvent>
{
public:

	explicit CAudioEvent(CATLEvent* pAudioEvent)
		: m_pAudioEvent(pAudioEvent)
	{}

	virtual ~CAudioEvent() override;

	CAudioEvent(CAudioEvent const&) = delete;
	CAudioEvent(CAudioEvent&&) = delete;
	CAudioEvent&                 operator=(CAudioEvent const&) = delete;
	CAudioEvent&                 operator=(CAudioEvent&&) = delete;

	bool                         PrepareForOcclusion();
	void                         SetObstructionOcclusion(float const obstruction, float const occlusion);
	CATLEvent&                   GetATLEvent() const                                       { return *m_pAudioEvent; }
	uint32                       GetEventPathId() const                                    { return m_eventPathId; }
	void                         SetEventPathId(uint32 const eventPathId)                  { m_eventPathId = eventPathId; }
	FMOD::Studio::EventInstance* GetInstance() const                                       { return m_pInstance; }
	void                         SetInstance(FMOD::Studio::EventInstance* const pInstance) { m_pInstance = pInstance; }
	CAudioObjectBase* const      GetAudioObject()                                          { return m_pAudioObject; }
	void                         SetAudioObject(CAudioObjectBase* const pAudioObject)      { m_pAudioObject = pAudioObject; }
	void                         TrySetEnvironment(CAudioEnvironment const* const pEnvironment, float const value);

	// IAudioEvent
	virtual ERequestStatus Stop() override;
	// ~IAudioEvent

private:

	CATLEvent*                       m_pAudioEvent = nullptr;
	uint32                           m_eventPathId = AUDIO_INVALID_CRC32;

	float                            m_lowpassFrequencyMax = 0.0f;
	float                            m_lowpassFrequencyMin = 0.0f;

	FMOD::Studio::EventInstance*     m_pInstance = nullptr;
	FMOD::ChannelGroup*              m_pMasterTrack = nullptr;
	FMOD::DSP*                       m_pLowpass = nullptr;
	FMOD::Studio::ParameterInstance* m_pOcclusionParameter = nullptr;
	CAudioObjectBase*                m_pAudioObject = nullptr;
};
}
}
}
