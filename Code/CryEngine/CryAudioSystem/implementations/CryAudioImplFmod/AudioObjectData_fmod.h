// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities_fmod.h"
#include <SharedAudioData.h>

namespace CryAudio
{
namespace Impl
{
class CAudioObject_fmod final : public IAudioObject
{
public:

	explicit CAudioObject_fmod(AudioObjectId const _id);
	virtual ~CAudioObject_fmod() {}

	AudioObjectId             GetId() const { return m_id; }
	void                      RemoveAudioEvent(CAudioEvent_fmod* const pAudioEvent);
	bool                      SetAudioEvent(CAudioEvent_fmod* const pAudioEvent);
	void                      RemoveParameter(CAudioParameter_fmod const* const pParameter);
	void                      SetParameter(CAudioParameter_fmod const* const pAudioParameter, float const value);
	void                      RemoveSwitch(CAudioSwitchState_fmod const* const pSwitch);
	void                      SetSwitch(CAudioSwitchState_fmod const* const pSwitch);
	void                      RemoveEnvironment(CAudioEnvironment_fmod const* const pEnvironment);
	void                      SetEnvironment(CAudioEnvironment_fmod const* const pEnvironment, float const value);
	void                      Set3DAttributes(SAudioObject3DAttributes const& attributes);
	void                      StopAllEvents();
	void                      StopEvent(uint32 const eventPathId);
	void                      SetObstructionOcclusion(float const obstruction, float const occlusion);
	FMOD_3D_ATTRIBUTES const& Get3DAttributes() const { return m_attributes; }
	void                      Reset();

private:

	AudioObjectId const m_id;
	FMOD_3D_ATTRIBUTES  m_attributes;

	AudioEvents         m_audioEvents;
	typedef std::map<CAudioParameter_fmod const* const, float>    AudioParameters;
	AudioParameters     m_audioParameters;
	typedef std::map<uint32 const, CAudioSwitchState_fmod const*> AudioSwitches;
	AudioSwitches       m_audioSwitches;
	typedef std::map<CAudioEnvironment_fmod const* const, float, std::less<CAudioEnvironment_fmod const* const>,
	                 STLSoundAllocator<std::pair<CAudioEnvironment_fmod const* const, float>>> AudioEnvironments;
	AudioEnvironments m_audioEnvironments;

	DELETE_DEFAULT_CONSTRUCTOR(CAudioObject_fmod);
	PREVENT_OBJECT_COPY(CAudioObject_fmod);
};
}
}
