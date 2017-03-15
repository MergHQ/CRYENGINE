// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CAudioEvent;

class CAudioObject final : public IAudioObject, public CPoolObject<CAudioObject, stl::PSyncNone>
{
public:

	CAudioObject() = default;
	virtual ~CAudioObject() override = default;

	CAudioObject(CAudioObject const&) = delete;
	CAudioObject(CAudioObject&&) = delete;
	CAudioObject& operator=(CAudioObject const&) = delete;
	CAudioObject& operator=(CAudioObject&&) = delete;

	// IAudioObject
	virtual ERequestStatus Update() override;
	virtual ERequestStatus Set3DAttributes(SObject3DAttributes const& attributes) override;
	virtual ERequestStatus SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount) override;
	virtual ERequestStatus SetParameter(IParameter const* const pIAudioParameter, float const value) override;
	virtual ERequestStatus SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState) override;
	virtual ERequestStatus SetObstructionOcclusion(float const obstruction, float const occlusion) override;
	virtual ERequestStatus ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent) override;
	virtual ERequestStatus StopAllTriggers() override;
	virtual ERequestStatus PlayFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus StopFile(IAudioStandaloneFile* const pIFile) override;
	virtual ERequestStatus SetName(char const* const szName) override;
	// ~AudioObject

	void StopAudioEvent(uint32 const pathId);
	void RegisterAudioEvent(CAudioEvent* const pEvent);
	void UnregisterAudioEvent(CAudioEvent* const pEvent);

private:

	std::vector<CAudioEvent*> m_activeEvents;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
