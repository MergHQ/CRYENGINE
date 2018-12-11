// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
class CEvent;

class CEventManager final
{
public:

	CEventManager() = default;
	~CEventManager();

	CEventManager(CEventManager const&) = delete;
	CEventManager& operator=(CEventManager const&) = delete;

	void           Initialize(uint32 const poolSize);
	void           OnAfterImplChanged();
	void           ReleaseImplData();
	void           Release();

	CEvent*        ConstructEvent();
	void           DestructEvent(CEvent* const pEvent);

private:

	std::vector<CEvent*> m_constructedEvents;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
public:
	size_t GetNumConstructed() const;
	void   DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY) const;

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
