// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
class CATLAudioObject;
class CAudioEventManager;
class CAudioStandaloneFileManager;

namespace Impl
{
struct SObject3DAttributes;
}

class CAudioObjectManager final
{
public:

	using ConstructedAudioObjectsList = std::list<CATLAudioObject*>;

	explicit CAudioObjectManager(CAudioEventManager& audioEventMgr, CAudioStandaloneFileManager& audioStandaloneFileMgr);
	~CAudioObjectManager();

	CAudioObjectManager(CAudioObjectManager const&) = delete;
	CAudioObjectManager(CAudioObjectManager&&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager const&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager&&) = delete;

	void                 Init(Impl::IAudioImpl* const pImpl);
	void                 Release();
	void                 Update(float const deltaTime, Impl::SObject3DAttributes const& listenerAttributes);
	CATLAudioObject*     ConstructAudioObject(char const* const szName);
	void                 ReleaseAudioObject(CATLAudioObject* const pAudioObject);

	void                 ReportStartedEvent(CATLEvent* const pEvent);
	void                 ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess);
	void                 GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request);
	void                 ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile);
	void                 ReleasePendingRays();
	bool                 IsActive(CATLAudioObject const* const pAudioObject) const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	size_t                             GetNumAudioObjects() const;
	size_t                             GetNumActiveAudioObjects() const;
	ConstructedAudioObjectsList const& GetAudioObjects() const { return m_constructedAudioObjects; }
	void                               DrawPerObjectDebugInfo(
	  IRenderAuxGeom& auxGeom,
	  Vec3 const& listenerPos,
	  AudioTriggerLookup const& triggers,
	  AudioParameterLookup const& parameters,
	  AudioSwitchLookup const& switches,
	  AudioPreloadRequestLookup const& preloadRequests,
	  AudioEnvironmentLookup const& environments) const;
	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	static float s_controlsUpdateInterval;

	bool HasActiveData(CATLAudioObject const* const pAudioObject) const;

	ConstructedAudioObjectsList  m_constructedAudioObjects;

	Impl::IAudioImpl*            m_pImpl;
	float                        m_timeSinceLastControlsUpdate;

	CAudioEventManager&          m_audioEventMgr;
	CAudioStandaloneFileManager& m_audioStandaloneFileMgr;
};
} // namespace CryAudio
