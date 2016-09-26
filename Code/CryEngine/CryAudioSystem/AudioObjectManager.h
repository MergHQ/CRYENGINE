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

	typedef std::unordered_map<AudioObjectId const, CATLAudioObject* const, std::hash<AudioObjectId>, std::equal_to<AudioObjectId>, STLSoundAllocator<std::pair<AudioObjectId const, CATLAudioObject* const>>>
	  RegisteredAudioObjectsMap;

	explicit CAudioObjectManager(CAudioEventManager& _audioEventMgr, CAudioStandaloneFileManager& _audioStandaloneFileMgr);
	~CAudioObjectManager();

	CAudioObjectManager(CAudioObjectManager const&) = delete;
	CAudioObjectManager(CAudioObjectManager&&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager const&) = delete;
	CAudioObjectManager& operator=(CAudioObjectManager&&) = delete;

	void                 Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                 Release();
	void                 Update(float const deltaTime, CryAudio::Impl::SAudioObject3DAttributes const& listenerAttributes);
	bool                 ReserveId(AudioObjectId& audioObjectId);
	bool                 ReserveThisId(AudioObjectId const audioObjectId);
	bool                 ReleaseId(AudioObjectId const audioObjectId);
	CATLAudioObject*     LookupId(AudioObjectId const audioObjectId) const;

	void                 ReportStartedEvent(CATLEvent* const pEvent);
	void                 ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess);
	void                 GetStartedStandaloneFileRequestData(CATLStandaloneFile* const _pStandaloneFile, CAudioRequestInternal& _request);
	void                 ReportFinishedStandaloneFile(CATLStandaloneFile* const _pStandaloneFile);
	void                 ReleasePendingRays();
	bool                 IsActive(CATLAudioObject const* const pAudioObject) const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)

	bool                             ReserveId(AudioObjectId& audioObjectId, char const* const szAudioObjectName);
	void                             SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore);
	size_t                           GetNumAudioObjects() const;
	size_t                           GetNumActiveAudioObjects() const;
	RegisteredAudioObjectsMap const& GetRegisteredAudioObjects() const { return m_registeredAudioObjects; }
	void                             DrawPerObjectDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPos) const;
	void                             DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

private:

	CATLDebugNameStore* m_pDebugNameStore;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	static float s_controlsUpdateInterval;

	CATLAudioObject* GetInstance();
	bool             ReleaseInstance(CATLAudioObject* const pOldObject);
	bool             HasActiveData(CATLAudioObject const* const pAudioObject) const;

	RegisteredAudioObjectsMap m_registeredAudioObjects;

	typedef CInstanceManager<CATLAudioObject, AudioObjectId> TAudioObjectPool;
	TAudioObjectPool             m_audioObjectPool;
	CryAudio::Impl::IAudioImpl*  m_pImpl;
	float                        m_timeSinceLastControlsUpdate;

	static AudioObjectId const   s_minAudioObjectId;

	CAudioEventManager&          m_audioEventMgr;
	CAudioStandaloneFileManager& m_audioStandaloneFileMgr;
};
