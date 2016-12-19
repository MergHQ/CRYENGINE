// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IAudioImpl;
struct SObject3DAttributes;
}

class CATLListener;

class CAudioListenerManager final
{
public:

	CAudioListenerManager() = default;
	~CAudioListenerManager();

	CAudioListenerManager(CAudioListenerManager const&) = delete;
	CAudioListenerManager(CAudioListenerManager&&) = delete;
	CAudioListenerManager&           operator=(CAudioListenerManager const&) = delete;
	CAudioListenerManager&           operator=(CAudioListenerManager&&) = delete;

	void                             Init(Impl::IAudioImpl* const pImpl);
	void                             Release();
	void                             Update(float const deltaTime);
	CATLListener*                    CreateListener();
	void                             ReleaseListener(CATLListener* const pListener);
	size_t                           GetNumActiveListeners() const;
	Impl::SObject3DAttributes const& GetActiveListenerAttributes() const;

private:

	std::vector<CATLListener*> m_activeListeners;
	Impl::IAudioImpl*          m_pImpl = nullptr;
};
} // namespace CryAudio
