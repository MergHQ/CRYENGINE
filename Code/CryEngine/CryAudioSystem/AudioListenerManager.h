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

class CATLListener;

class CAudioListenerManager final
{
public:

	CAudioListenerManager() = default;
	~CAudioListenerManager();

	CAudioListenerManager(CAudioListenerManager const&) = delete;
	CAudioListenerManager(CAudioListenerManager&&) = delete;
	CAudioListenerManager&                          operator=(CAudioListenerManager const&) = delete;
	CAudioListenerManager&                          operator=(CAudioListenerManager&&) = delete;

	void                                            Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                                            Release();
	void                                            Update(float const deltaTime);
	CATLListener*                                   CreateListener();
	void                                            Release(CATLListener* const pListener);
	size_t                                          GetNumActive() const;
	CryAudio::Impl::SAudioObject3DAttributes const& GetDefaultListenerAttributes() const;
	CATLListener*                                   GetDefaultListener() const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	float GetDefaultListenerVelocity() const;
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

private:

	std::vector<CATLListener*, STLSoundAllocator<CATLListener*>> m_activeListeners;
	CATLListener*               m_pDefaultListenerObject = nullptr;

	CryAudio::Impl::IAudioImpl* m_pImpl = nullptr;
};
