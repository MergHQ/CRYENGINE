// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Math.h>

namespace CryAudio
{
class CListener;
class CTransformation;

class CListenerManager final
{
public:

	CListenerManager() = default;
	~CListenerManager();

	CListenerManager(CListenerManager const&) = delete;
	CListenerManager(CListenerManager&&) = delete;
	CListenerManager&      operator=(CListenerManager const&) = delete;
	CListenerManager&      operator=(CListenerManager&&) = delete;

	void                   Terminate();
	void                   OnAfterImplChanged();
	void                   ReleaseImplData();
	void                   Update(float const deltaTime);
	CListener*             CreateListener(CTransformation const& transformation, char const* const szName);
	void                   ReleaseListener(CListener* const pListener);
	CTransformation const& GetActiveListenerTransformation() const;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	size_t GetNumActiveListeners() const;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

private:

	std::vector<CListener*> m_constructedListeners;
};
} // namespace CryAudio
