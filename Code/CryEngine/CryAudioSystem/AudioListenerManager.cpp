// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioListenerManager.h"
#include "ATLEntities.h"
#include <ATLEntityData.h>
#include <IAudioImpl.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioListenerManager::CAudioListenerManager()
	: m_numListeners(8)
	, m_pDefaultListenerObject(nullptr)
	, m_defaultListenerId(50) // IDs 50-57 are reserved for the AudioListeners.
	, m_pImpl(nullptr)
{
	m_listenerPool.reserve(static_cast<size_t>(m_numListeners));
}

//////////////////////////////////////////////////////////////////////////
CAudioListenerManager::~CAudioListenerManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}

	stl::free_container(m_listenerPool);
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;
	IAudioListener* pAudioListener = m_pImpl->NewDefaultAudioListener(m_defaultListenerId);
	POOL_NEW(CATLListenerObject, m_pDefaultListenerObject)(m_defaultListenerId, pAudioListener);

	if (m_pDefaultListenerObject != nullptr)
	{
		m_activeListeners[m_defaultListenerId] = m_pDefaultListenerObject;
	}

	for (AudioObjectId i = 1; i < m_numListeners; ++i)
	{
		AudioObjectId const listenerId = m_defaultListenerId + i;
		pAudioListener = m_pImpl->NewAudioListener(listenerId);
		POOL_NEW_CREATE(CATLListenerObject, pListenerObject)(listenerId, pAudioListener);
		m_listenerPool.push_back(pListenerObject);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Release()
{
	if (m_pDefaultListenerObject != nullptr)
	{
		m_activeListeners.erase(m_defaultListenerId);
		m_pImpl->DeleteAudioListener(m_pDefaultListenerObject->m_pImplData);
		POOL_FREE(m_pDefaultListenerObject);
		m_pDefaultListenerObject = nullptr;
	}

	for (auto const pListener : m_listenerPool)
	{
		m_pImpl->DeleteAudioListener(pListener->m_pImplData);
		POOL_FREE(pListener);
	}

	m_listenerPool.clear();
	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Update(float const deltaTime)
{
	if (m_pDefaultListenerObject != nullptr)
	{
		m_pDefaultListenerObject->Update(m_pImpl);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAudioListenerManager::ReserveId(AudioObjectId& audioObjectId)
{
	bool bSuccess = false;

	if (!m_listenerPool.empty())
	{
		CATLListenerObject* pListener = m_listenerPool.back();
		m_listenerPool.pop_back();

		AudioObjectId const id = pListener->GetId();
		m_activeListeners.insert(std::make_pair(id, pListener));

		audioObjectId = id;
		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioListenerManager::ReleaseId(AudioObjectId const audioObjectId)
{
	bool bSuccess = false;
	CATLListenerObject* pListener = LookupId(audioObjectId);

	if (pListener != nullptr)
	{
		m_activeListeners.erase(audioObjectId);
		m_listenerPool.push_back(pListener);
		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
CATLListenerObject* CAudioListenerManager::LookupId(AudioObjectId const audioObjectId) const
{
	CATLListenerObject* pListenerObject = nullptr;

	TActiveListenerMap::const_iterator iPlace = m_activeListeners.begin();

	if (FindPlaceConst(m_activeListeners, audioObjectId, iPlace))
	{
		pListenerObject = iPlace->second;
	}

	return pListenerObject;
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioListenerManager::GetNumActive() const
{
	return m_activeListeners.size();
}

//////////////////////////////////////////////////////////////////////////
SAudioObject3DAttributes const& CAudioListenerManager::GetDefaultListenerAttributes() const
{
	if (m_pDefaultListenerObject != nullptr)
	{
		return m_pDefaultListenerObject->Get3DAttributes();
	}

	return g_sNullAudioObjectAttributes;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
float CAudioListenerManager::GetDefaultListenerVelocity() const
{
	float velocity = 0.0f;

	if (m_pDefaultListenerObject != nullptr)
	{
		velocity = m_pDefaultListenerObject->GetVelocity();
	}

	return velocity;
}
#endif //INCLUDE_AUDIO_PRODUCTION_CODE
