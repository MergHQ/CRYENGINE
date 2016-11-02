// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"

class CAudioEventManager final
{
public:

	CAudioEventManager();
	~CAudioEventManager();

	CAudioEventManager(CAudioEventManager const&) = delete;
	CAudioEventManager(CAudioEventManager&&) = delete;
	CAudioEventManager& operator=(CAudioEventManager const&) = delete;
	CAudioEventManager& operator=(CAudioEventManager&&) = delete;

	void                Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                Release();
	void                Update(float const deltaTime);

	CATLEvent*          GetEvent(EAudioSubsystem const audioSubsystem);
	CATLEvent*          LookupId(AudioEventId const audioEventId) const;
	void                ReleaseEvent(CATLEvent* const pEvent);

	size_t              GetNumActive() const;

private:

	CATLEvent* GetImplInstance();
	void       ReleaseEventInternal(CATLEvent* const pEvent);
	void       ReleaseImplInstance(CATLEvent* const pOldEvent);
	CATLEvent* GetInternalInstance();
	void       ReleaseInternalInstance(CATLEvent* const pOldEvent);

	typedef std::map<AudioEventId, CATLEvent*, std::less<AudioEventId>, STLSoundAllocator<std::pair<AudioEventId const, CATLEvent*>>>
	  TActiveEventMap;

	TActiveEventMap m_activeAudioEvents;

	typedef CInstanceManager<CATLEvent, AudioEventId> TAudioEventPool;
	TAudioEventPool             m_audioEventPool;

	CryAudio::Impl::IAudioImpl* m_pImpl;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE
};
