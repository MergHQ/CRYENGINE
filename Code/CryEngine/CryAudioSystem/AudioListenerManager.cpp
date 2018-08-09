// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioListenerManager.h"
#include "ATLEntities.h"
#include "Common.h"
#include <ATLEntityData.h>
#include <IAudioImpl.h>
#include <algorithm>

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CAudioListenerManager::~CAudioListenerManager()
{
	CRY_ASSERT_MESSAGE(m_constructedListeners.empty(), "There are still listeners during CAudioListenerManager destruction!");
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Terminate()
{
	for (auto const pListener : m_constructedListeners)
	{
		CRY_ASSERT_MESSAGE(pListener->m_pImplData == nullptr, "A listener cannot have valid impl data during CAudioListenerManager destruction!");
		delete pListener;
	}

	m_constructedListeners.clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::OnAfterImplChanged()
{
	if (!m_constructedListeners.empty())
	{
		for (auto const pListener : m_constructedListeners)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pListener->m_pImplData = g_pIImpl->ConstructListener(pListener->m_name.c_str());
#else
			pListener->m_pImplData = g_pIImpl->ConstructListener();
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pListener->HandleSetTransformation(pListener->GetTransformation());
		}
	}
	else
	{
		// Create a default listener for early functionality.
		CreateListener("DefaultListener");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::ReleaseImplData()
{
	for (auto const pListener : m_constructedListeners)
	{
		g_pIImpl->DestructListener(pListener->m_pImplData);
		pListener->m_pImplData = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Update(float const deltaTime)
{
	if (deltaTime > 0.0f)
	{
		for (auto const pListener : m_constructedListeners)
		{
			pListener->Update(deltaTime);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CATLListener* CAudioListenerManager::CreateListener(char const* const szName /*= nullptr*/)
{
	if (!m_constructedListeners.empty())
	{
		// Currently only one listener supported!
		CATLListener* const pListener = m_constructedListeners.front();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		// Update name, TODO: needs reconstruction with the middleware in order to update it as well.
		pListener->m_name = szName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

		return pListener;
	}

	auto const pListener = new CATLListener(g_pIImpl->ConstructListener(szName));

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pListener->m_name = szName;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_constructedListeners.push_back(pListener);
	return pListener;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::ReleaseListener(CATLListener* const pListener)
{
	// As we currently support only one listener we will destroy that instance only on engine shutdown!
	/*m_constructedListeners.erase
	   (
	   std::find_if(m_constructedListeners.begin(), m_constructedListeners.end(), [=](CATLListener const* pRegisteredListener)
	   {
	   if (pRegisteredListener == pListener)
	   {
	    g_pIImpl->DestructListener(pListener->m_pImplData);
	    delete pListener;
	    return true;
	   }

	   return false;
	   })
	   );*/
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioListenerManager::GetNumActiveListeners() const
{
	return m_constructedListeners.size();
}

//////////////////////////////////////////////////////////////////////////
CObjectTransformation const& CAudioListenerManager::GetActiveListenerTransformation() const
{
	for (auto const pListener : m_constructedListeners)
	{
		// Only one listener supported currently!
		return pListener->GetTransformation();
	}

	return CObjectTransformation::GetEmptyObject();
}

//////////////////////////////////////////////////////////////////////////
Vec3 const& CAudioListenerManager::GetActiveListenerVelocity() const
{
	for (auto const pListener : m_constructedListeners)
	{
		// Only one listener supported currently!
		return pListener->GetVelocity();
	}

	return Vec3Constants<float>::fVec3_Zero;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
char const* CAudioListenerManager::GetActiveListenerName() const
{
	for (auto const pListener : m_constructedListeners)
	{
		// Only one listener supported currently!
		return pListener->m_name.c_str();
	}

	return nullptr;
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
