// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntityData.h"
#include <SharedAudioData.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CAudioEvent;
class CAudioParameter;
class CAudioSwitchState;
class CAudioEnvironment;
class CAudioFileBase;

class CAudioObjectBase : public IAudioObject
{
public:

	CAudioObjectBase();
	virtual ~CAudioObjectBase();

	void RemoveEvent(CAudioEvent* const pAudioEvent);
	void RemoveParameter(CAudioParameter const* const pParameter);
	void RemoveSwitch(CAudioSwitchState const* const pSwitch);
	void RemoveEnvironment(CAudioEnvironment const* const pEnvironment);
	void RemoveFile(CAudioFileBase const* const pStandaloneFile);

	// IAudioObject
	virtual ERequestStatus Update() override;
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override;
	virtual ERequestStatus StopAllTriggers() override;
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus SetName(char const* const szName) override;
	// ~IAudioObject

protected:

	void StopEvent(uint32 const eventPathId);
	bool SetAudioEvent(CAudioEvent* const pAudioEvent);

	std::vector<CAudioEvent*>                        m_audioEvents;
	std::vector<CAudioFileBase*>                     m_files;
	std::map<CAudioParameter const* const, float>    m_audioParameters;
	std::map<uint32 const, CAudioSwitchState const*> m_audioSwitches;
	std::map<CAudioEnvironment const* const, float>  m_audioEnvironments;

	std::vector<CAudioEvent*>                        m_pendingAudioEvents;
	std::vector<CAudioFileBase*>                     m_pendingFiles;

	FMOD_3D_ATTRIBUTES                               m_attributes;
	float m_obstruction = 0.0f;
	float m_occlusion = 0.0f;

public:

	static FMOD::Studio::System* s_pSystem;
};

using AudioObjects = std::vector<CAudioObjectBase*>;

class CAudioObject final : public CAudioObjectBase, public CPoolObject<CAudioObject, stl::PSyncNone>
{
public:

	// IAudioObject
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override;
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override;
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override;
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	// ~IAudioObject
};

class CGlobalAudioObject final : public CAudioObjectBase
{
public:

	CGlobalAudioObject(AudioObjects const& audioObjectsList)
		: m_audioObjectsList(audioObjectsList)
	{}

	// IAudioObject
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override;
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override;
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override;
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	// ~IAudioObject

	AudioObjects const& m_audioObjectsList;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
