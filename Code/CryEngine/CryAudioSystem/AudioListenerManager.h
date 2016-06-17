// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IAudioImpl;
struct SAudioObject3DAttributes;
}
}

class CATLListenerObject;

class CAudioListenerManager
{
public:

	CAudioListenerManager();
	~CAudioListenerManager();

	void                                            Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                                            Release();
	void                                            Update(float const deltaTime);
	bool                                            ReserveId(AudioObjectId& audioObjectId);
	bool                                            ReleaseId(AudioObjectId const audioObjectId);
	CATLListenerObject*                             LookupId(AudioObjectId const audioObjectId) const;
	size_t                                          GetNumActive() const;
	CryAudio::Impl::SAudioObject3DAttributes const& GetDefaultListenerAttributes() const;
	AudioObjectId                                   GetDefaultListenerId() const { return m_defaultListenerId; }

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	float GetDefaultListenerVelocity() const;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	typedef std::vector<CATLListenerObject*, STLSoundAllocator<CATLListenerObject*>> TListenerPtrContainer;
	typedef std::map<AudioObjectId, CATLListenerObject*, std::less<AudioObjectId>, STLSoundAllocator<std::pair<AudioObjectId, CATLListenerObject*>>>
	  TActiveListenerMap;

	TActiveListenerMap          m_activeListeners;
	TListenerPtrContainer       m_listenerPool;
	CATLListenerObject*         m_pDefaultListenerObject;
	AudioObjectId const         m_defaultListenerId;
	AudioObjectId               m_numListeners;
	CryAudio::Impl::IAudioImpl* m_pImpl;

	PREVENT_OBJECT_COPY(CAudioListenerManager);
};
