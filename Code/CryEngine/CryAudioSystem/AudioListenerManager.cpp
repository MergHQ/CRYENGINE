// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioListenerManager.h"
#include "ATLEntities.h"
#include <ATLEntityData.h>
#include <IAudioImpl.h>
#include <algorithm>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioListenerManager::~CAudioListenerManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	CRY_ASSERT(m_activeListeners.empty() && !m_pDefaultListenerObject);

	POOL_NEW(CATLListener, m_pDefaultListenerObject)(m_pImpl->NewDefaultAudioListener());
	m_activeListeners.push_back(m_pDefaultListenerObject);
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Release()
{
	for (auto pListener : m_activeListeners)
	{
		m_pImpl->DeleteAudioListener(pListener->m_pImplData);
		POOL_FREE(pListener);
	}
	m_activeListeners.clear();

	m_pDefaultListenerObject = nullptr;

	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Update(float const deltaTime)
{
	CRY_ASSERT(m_pDefaultListenerObject != nullptr);
	m_pDefaultListenerObject->Update(m_pImpl);
}

//////////////////////////////////////////////////////////////////////////
CATLListener* CAudioListenerManager::CreateListener()
{
	POOL_NEW_CREATE(CATLListener, pListener)(m_pImpl->NewAudioListener());
	m_activeListeners.push_back(pListener);
	return pListener;
}

//////////////////////////////////////////////////////////////////////////
void CAudioListenerManager::Release(CATLListener* const pListener)
{
	m_activeListeners.erase
	(
	  std::find_if(m_activeListeners.begin(), m_activeListeners.end(), [=](CATLListener const* pRegisteredListener) { return pRegisteredListener == pListener; })
	);
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

//////////////////////////////////////////////////////////////////////////
CATLListener* CAudioListenerManager::GetDefaultListener() const
{
	return m_pDefaultListenerObject;
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
