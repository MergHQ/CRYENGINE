// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ListenerManager.h"
#include "Common.h"
#include "Listener.h"
#include <IImpl.h>

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CListenerManager::~CListenerManager()
{
	CRY_ASSERT_MESSAGE(m_constructedListeners.empty(), "There are still listeners during %s", __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////
void CListenerManager::Terminate()
{
	for (auto const pListener : m_constructedListeners)
	{
		CRY_ASSERT_MESSAGE(pListener->m_pImplData == nullptr, "A listener cannot have valid impl data during %s", __FUNCTION__);
		delete pListener;
	}

	m_constructedListeners.clear();
}

//////////////////////////////////////////////////////////////////////////
void CListenerManager::OnAfterImplChanged()
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
		CreateListener(CTransformation::GetEmptyObject(), "DefaultListener");
	}
#else
	CreateListener(CTransformation::GetEmptyObject(), "DefaultListener");
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CListenerManager::ReleaseImplData()
{
	for (auto const pListener : m_constructedListeners)
	{
		g_pIImpl->DestructListener(pListener->m_pImplData);
		pListener->m_pImplData = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CListenerManager::Update(float const deltaTime)
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
CListener* CListenerManager::CreateListener(CTransformation const& transformation, char const* const szName)
{
	if (!m_constructedListeners.empty())
	{
		// Currently only one listener supported!
		CListener* const pListener = m_constructedListeners.front();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pListener->SetName(szName);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

		return pListener;
	}

	auto const pListener = new CListener(g_pIImpl->ConstructListener(transformation, szName));

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pListener->SetName(szName);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_constructedListeners.push_back(pListener);
	return pListener;
}

//////////////////////////////////////////////////////////////////////////
void CListenerManager::ReleaseListener(CListener* const pListener)
{
	// As we currently support only one listener we will destroy that instance only on engine shutdown!
	/*m_constructedListeners.erase
	   (
	   std::find_if(m_constructedListeners.begin(), m_constructedListeners.end(), [=](CListener const* pRegisteredListener)
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
CTransformation const& CListenerManager::GetActiveListenerTransformation() const
{
	for (auto const pListener : m_constructedListeners)
	{
		// Only one listener supported currently!
		return pListener->GetTransformation();
	}

	return CTransformation::GetEmptyObject();
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
size_t CListenerManager::GetNumActiveListeners() const
{
	return m_constructedListeners.size();
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
