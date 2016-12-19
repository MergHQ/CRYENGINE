// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"

namespace CryAudio
{
class CAudioEventManager final
{
public:

	CAudioEventManager() = default;
	~CAudioEventManager();

	CAudioEventManager(CAudioEventManager const&) = delete;
	CAudioEventManager(CAudioEventManager&&) = delete;
	CAudioEventManager& operator=(CAudioEventManager const&) = delete;
	CAudioEventManager& operator=(CAudioEventManager&&) = delete;

	void                Init(Impl::IAudioImpl* const pImpl);
	void                Release();
	void                Update(float const deltaTime);

	CATLEvent*          ConstructAudioEvent();
	void                ReleaseAudioEvent(CATLEvent* const pAudioEvent);

	size_t              GetNumConstructed() const;

private:

	std::list<CATLEvent*> m_constructedAudioEvents;

	Impl::IAudioImpl*     m_pImpl = nullptr;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:

	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const;

#endif //INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
