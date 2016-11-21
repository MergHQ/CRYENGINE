// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

class CATLAudioObject;
class CAudioEventManager;
class CAudioStandaloneFileManager;

namespace CryAudio
{
namespace Impl
{
struct SAudioObject3DAttributes;
}
}

class CAudioObjectManager final
{
public:

	using RegisteredObjects = std::list<CATLAudioObject*>;

	explicit CAudioObjectManager(CAudioEventManager& audioEventMgr, CAudioStandaloneFileManager& audioStandaloneFileMgr);
	~CAudioObjectManager();

	CAudioObjectManager(CAudioObjectManager const&) = delete;
	CAudioObjectManager(CAudioObjectManager&&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager const&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager&&) = delete;

	void                 Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                 Release();
	void                 Update(float const deltaTime, CryAudio::Impl::SAudioObject3DAttributes const& listenerAttributes);
	bool                 ReserveAudioObject(CATLAudioObject*& outAudioObject);
	bool                 ReleaseAudioObject(CATLAudioObject* const pAudioObject);

	void                 ReportStartedEvent(CATLEvent* const pEvent);
	void                 ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess);
	void                 GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequestInternal& request);
	void                 ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile);
	void                 ReleasePendingRays();
	bool                 IsActive(CATLAudioObject const* const pAudioObject) const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)

	bool                     ReserveAudioObject(CATLAudioObject*& outAudioObject, char const* const szAudioObjectName);
	size_t                   GetNumAudioObjects() const;
	size_t                   GetNumActiveAudioObjects() const;
	RegisteredObjects const& GetRegisteredAudioObjects() const { return m_registeredAudioObjects; }
	void                     DrawPerObjectDebugInfo(
	  IRenderAuxGeom& auxGeom,
	  Vec3 const& listenerPos,
	  AudioTriggerLookup const& triggers,
	  AudioRtpcLookup const& parameters,
	  AudioSwitchLookup const& switches,
	  AudioPreloadRequestLookup const& preloadRequests,
	  AudioEnvironmentLookup const& environments,
	  AudioStandaloneFileLookup const& audioStandaloneFiles) const;
	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	static float s_controlsUpdateInterval;

	CATLAudioObject* GetInstance();
	bool             ReleaseInstance(CATLAudioObject* const pOldObject);
	bool             HasActiveData(CATLAudioObject const* const pAudioObject) const;

	RegisteredObjects m_registeredAudioObjects;

	typedef CInstanceManager<CATLAudioObject, AudioIdType> TAudioObjectPool;
	TAudioObjectPool             m_audioObjectPool;
	CryAudio::Impl::IAudioImpl*  m_pImpl;
	float                        m_timeSinceLastControlsUpdate;

	CAudioEventManager&          m_audioEventMgr;
	CAudioStandaloneFileManager& m_audioStandaloneFileMgr;
};
