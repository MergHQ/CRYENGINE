// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioListenerManager.h"
#include "ATLEntities.h"
#include <IAudioImpl.h>

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
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (!m_constructedListeners.empty())
	{
		for (auto const pListener : m_constructedListeners)
		{
			pListener->m_pImplData = g_pIImpl->ConstructListener(pListener->GetDebugTransformation(), pListener->GetName());
		}
	}
	else
	{
		CreateListener(CObjectTransformation::GetEmptyObject(), "DefaultListener");
	}
#else
	CreateListener(CObjectTransformation::GetEmptyObject(), "DefaultListener");
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
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
CATLListener* CAudioListenerManager::CreateListener(CObjectTransformation const& transformation, char const* const szName)
{
	if (!m_constructedListeners.empty())
	{
		// Currently only one listener supported!
		CATLListener* const pListener = m_constructedListeners.front();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pListener->SetName(szName);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

		return pListener;
	}

	auto const pListener = new CATLListener(g_pIImpl->ConstructListener(transformation, szName));

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pListener->SetName(szName);
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
CObjectTransformation const& CAudioListenerManager::GetActiveListenerTransformation() const
{
	for (auto const pListener : m_constructedListeners)
	{
		// Only one listener supported currently!
		return pListener->GetTransformation();
	}

	return CObjectTransformation::GetEmptyObject();
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
size_t CAudioListenerManager::GetNumActiveListeners() const
{
	return m_constructedListeners.size();
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
