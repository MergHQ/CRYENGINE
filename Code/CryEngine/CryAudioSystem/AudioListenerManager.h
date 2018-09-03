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
	CATLListener*                CreateListener(CObjectTransformation const& transformation, char const* const szName);
	void                         ReleaseListener(CATLListener* const pListener);
	CObjectTransformation const& GetActiveListenerTransformation() const;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	size_t GetNumActiveListeners() const;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	std::vector<CATLListener*> m_constructedListeners;
};
} // namespace CryAudio
