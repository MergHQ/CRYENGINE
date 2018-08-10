// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Math.h>

namespace CryAudio
{
class CATLListener;
class CObjectTransformation;

class CAudioListenerManager final
{
public:

	CAudioListenerManager() = default;
	~CAudioListenerManager();

	CAudioListenerManager(CAudioListenerManager const&) = delete;
	CAudioListenerManager(CAudioListenerManager&&) = delete;
	CAudioListenerManager&       operator=(CAudioListenerManager const&) = delete;
	CAudioListenerManager&       operator=(CAudioListenerManager&&) = delete;

	void                         Terminate();
	void                         OnAfterImplChanged();
	void                         ReleaseImplData();
	void                         Update(float const deltaTime);
	CATLListener*                CreateListener(char const* const szName = nullptr);
	void                         ReleaseListener(CATLListener* const pListener);
	size_t                       GetNumActiveListeners() const;
	CObjectTransformation const& GetActiveListenerTransformation() const;
	Vec3 const&                  GetActiveListenerVelocity() const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	char const* GetActiveListenerName() const;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	std::vector<CATLListener*> m_constructedListeners;
};
} // namespace CryAudio
