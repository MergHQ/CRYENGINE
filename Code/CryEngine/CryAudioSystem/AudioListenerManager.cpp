// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioListenerManager.h"
#include "ATLEntities.h"
#include <ATLEntityData.h>
#include <IAudioImpl.h>
#include <algorithm>

using namespace CryAudio;
using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioListenerManager::~CAudioListenerManager()
{
	if (!m_activeListeners.empty())
	{
		for (auto pListener : m_activeListeners)
		{
			CRY_ASSERT(pListener->m_pImplData == nullptr);
			delete pListener;
		}

		m_activeListeners.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	if (!m_activeListeners.empty())
	{
		for (auto pListener : m_activeListeners)
		{
			pListener->m_pImplData = m_pImpl->ConstructAudioListener();
			pListener->HandleSetTransformation(pListener->Get3DAttributes().transformation);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Release()
{
	for (auto pListener : m_activeListeners)
	{
		m_pImpl->DestructAudioListener(pListener->m_pImplData);
		pListener->m_pImplData = nullptr;
	}

	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Update(float const deltaTime)
{
	for (auto pListener : m_activeListeners)
	{
		pListener->Update();
	}
}

//////////////////////////////////////////////////////////////////////////
CATLListener* CAudioListenerManager::CreateListener()
{
	if (!m_activeListeners.empty())
	{
		// Currently only one listener supported!
		return m_activeListeners.front();
	}

	CATLListener* const pListener = new CATLListener(m_pImpl->ConstructAudioListener());
	m_activeListeners.push_back(pListener);
	return pListener;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::ReleaseListener(CATLListener* const pListener)
{
	// As we currently support only one listener we will destroy that instance only on engine shutdown!
	/*m_activeListeners.erase
	(
	  std::find_if(m_activeListeners.begin(), m_activeListeners.end(), [=](CATLListener const* pRegisteredListener)
	{
		if (pRegisteredListener == pListener)
		{
			m_pImpl->DestructAudioListener(pListener->m_pImplData);
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
	return m_activeListeners.size();
}

//////////////////////////////////////////////////////////////////////////
SObject3DAttributes const& CAudioListenerManager::GetActiveListenerAttributes() const
{
	for (auto pListener : m_activeListeners)
	{
		// Only one listener supported currently!
		return pListener->Get3DAttributes();
	}

	return g_sNullAudioObjectAttributes;
}
