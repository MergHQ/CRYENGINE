// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioEventManager.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioEventManager::CAudioEventManager()
	: m_audioEventPool(g_audioCVars.m_audioEventPoolSize, 1)
	, m_pImpl(nullptr)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	, m_pDebugNameStore(nullptr)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
{
}

//////////////////////////////////////////////////////////////////////////
CAudioEventManager::~CAudioEventManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}

	if (!m_audioEventPool.m_reserved.empty())
	{
		for (auto const pEvent : m_audioEventPool.m_reserved)
		{
			CRY_ASSERT(pEvent->m_pImplData == nullptr);
			POOL_FREE(pEvent);
		}

		m_audioEventPool.m_reserved.clear();
	}

	if (!m_activeAudioEvents.empty())
	{
		for (auto const& eventPair : m_activeAudioEvents)
		{
			CATLEvent* const pEvent = eventPair.second;
			CRY_ASSERT(pEvent->m_pImplData == nullptr);
			POOL_FREE(pEvent);
		}

		m_activeAudioEvents.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	size_t const numPooledAudioEvents = m_audioEventPool.m_reserved.size();
	size_t const numActiveAudioEvents = m_activeAudioEvents.size();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if ((numPooledAudioEvents + numActiveAudioEvents) > std::numeric_limits<size_t>::max())
	{
		CryFatalError("Exceeding numeric limits during CAudioEventManager::Init");
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	size_t const numTotalEvents = numPooledAudioEvents + numActiveAudioEvents;
	CRY_ASSERT(!(numTotalEvents > 0 && numTotalEvents < m_audioEventPool.m_reserveSize));

	if (m_audioEventPool.m_reserveSize > numTotalEvents)
	{
		for (size_t i = 0; i < m_audioEventPool.m_reserveSize - numActiveAudioEvents; ++i)
		{
			AudioEventId const audioEventId = m_audioEventPool.GetNextID();
			POOL_NEW_CREATE(CATLEvent, pNewEvent)(audioEventId, eAudioSubsystem_AudioImpl);
			m_audioEventPool.m_reserved.push_back(pNewEvent);
		}
	}

	for (auto const pEvent : m_audioEventPool.m_reserved)
	{
		CRY_ASSERT(pEvent->m_pImplData == nullptr);
		pEvent->m_pImplData = m_pImpl->NewAudioEvent(pEvent->GetId());
	}

	for (auto const& eventPair : m_activeAudioEvents)
	{
		CATLEvent* const pEvent = eventPair.second;
		CRY_ASSERT(pEvent->m_pImplData == nullptr);
		pEvent->m_pImplData = m_pImpl->NewAudioEvent(pEvent->GetId());
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::Release()
{
	if (!m_activeAudioEvents.empty())
	{
		for (auto const& eventPair : m_activeAudioEvents)
		{
			ReleaseEventInternal(eventPair.second);
		}

		m_activeAudioEvents.clear();
	}

	for (auto const pEvent : m_audioEventPool.m_reserved)
	{
		m_pImpl->DeleteAudioEvent(pEvent->m_pImplData);
		pEvent->m_pImplData = nullptr;
	}

	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::Update(float const deltaTime)
{
	//TODO: implement
}

//////////////////////////////////////////////////////////////////////////
CATLEvent* CAudioEventManager::GetEvent(EAudioSubsystem const audioSubsystem)
{
	CATLEvent* pATLEvent = nullptr;

	switch (audioSubsystem)
	{
	case eAudioSubsystem_AudioImpl:
		{
			pATLEvent = GetImplInstance();

			break;
		}
	case eAudioSubsystem_AudioInternal:
		{
			pATLEvent = GetInternalInstance();

			break;
		}
	default:
		{
			CRY_ASSERT(false); // Unknown sender!
		}
	}

	if (pATLEvent != nullptr)
	{
		m_activeAudioEvents[pATLEvent->GetId()] = pATLEvent;
	}

	return pATLEvent;
}

//////////////////////////////////////////////////////////////////////////
CATLEvent* CAudioEventManager::LookupId(AudioEventId const audioEventId) const
{
	TActiveEventMap::const_iterator iter(m_activeAudioEvents.begin());
	bool const bLookupResult = FindPlaceConst(m_activeAudioEvents, audioEventId, iter);

	return bLookupResult ? iter->second : nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::ReleaseEvent(CATLEvent* const pEvent)
{
	m_activeAudioEvents.erase(pEvent->GetId());
	ReleaseEventInternal(pEvent);
}

//////////////////////////////////////////////////////////////////////////
CATLEvent* CAudioEventManager::GetImplInstance()
{
	CATLEvent* pEvent = nullptr;

	if (!m_audioEventPool.m_reserved.empty())
	{
		pEvent = m_audioEventPool.m_reserved.back();
		m_audioEventPool.m_reserved.pop_back();
	}
	else
	{
		AudioEventId const nNewID = m_audioEventPool.GetNextID();
		POOL_NEW(CATLEvent, pEvent)(nNewID, eAudioSubsystem_AudioImpl);

		if (pEvent != nullptr)
		{
			pEvent->m_pImplData = m_pImpl->NewAudioEvent(nNewID);
		}
		else
		{
			--m_audioEventPool.m_idCounter;
			g_audioLogger.Log(eAudioLogType_Warning, "Failed to get a new instance of an AudioEvent from the implementation");
		}
	}

	return pEvent;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::ReleaseEventInternal(CATLEvent* const pEvent)
{
	switch (pEvent->m_audioSubsystem)
	{
	case eAudioSubsystem_AudioImpl:
		ReleaseImplInstance(pEvent);
		break;
	case eAudioSubsystem_AudioInternal:
		ReleaseInternalInstance(pEvent);
		break;
	default:
		// Unknown sender.
		CRY_ASSERT(false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::ReleaseImplInstance(CATLEvent* const pOldEvent)
{
	if (pOldEvent != nullptr)
	{
		pOldEvent->Clear();
		m_pImpl->ResetAudioEvent(pOldEvent->m_pImplData);

		if (m_audioEventPool.m_reserved.size() < m_audioEventPool.m_reserveSize)
		{
			// can return the instance to the reserved pool
			m_audioEventPool.m_reserved.push_back(pOldEvent);
		}
		else
		{
			// the reserve pool is full, can return the instance to the implementation to dispose
			m_pImpl->DeleteAudioEvent(pOldEvent->m_pImplData);

			POOL_FREE(pOldEvent);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CATLEvent* CAudioEventManager::GetInternalInstance()
{
	// must be called within a block protected by a critical section!

	CRY_ASSERT(false);// implement when it is needed
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::ReleaseInternalInstance(CATLEvent* const pOldEvent)
{
	// must be called within a block protected by a critical section!

	CRY_ASSERT(false);// implement when it is needed
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioEventManager::GetNumActive() const
{
	return m_activeAudioEvents.size();
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const
{
	static float const headerColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
	static float const itemPlayingColor[4] = { 0.1f, 0.6f, 0.1f, 0.9f };
	static float const itemLoadingColor[4] = { 0.9f, 0.2f, 0.2f, 0.9f };
	static float const itemVirtualColor[4] = { 0.1f, 0.8f, 0.8f, 0.9f };
	static float const itemOtherColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };

	auxGeom.Draw2dLabel(posX, posY, 1.6f, headerColor, false, "Audio Events [%" PRISIZE_T "]", m_activeAudioEvents.size());
	posX += 20.0f;
	posY += 17.0f;

	for (auto const& eventPair : m_activeAudioEvents)
	{
		CATLEvent* const pEvent = eventPair.second;
		if (pEvent->m_pTrigger != nullptr)
		{
			char const* const szOriginalName = m_pDebugNameStore->LookupAudioTriggerName(pEvent->m_pTrigger->GetId());
			CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sLowerCaseAudioTriggerName(szOriginalName);
			sLowerCaseAudioTriggerName.MakeLower();
			CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sLowerCaseSearchString(g_audioCVars.m_pAudioTriggersDebugFilter->GetString());
			sLowerCaseSearchString.MakeLower();
			bool const bDraw = (sLowerCaseSearchString.empty() || (sLowerCaseSearchString.compareNoCase("0") == 0)) || (sLowerCaseAudioTriggerName.find(sLowerCaseSearchString) != CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH>::npos);

			if (bDraw)
			{
				float const* pColor = itemOtherColor;

				if (pEvent->IsPlaying())
				{
					pColor = itemPlayingColor;
				}
				else if (pEvent->m_audioEventState == eAudioEventState_Loading)
				{
					pColor = itemLoadingColor;
				}
				else if (pEvent->m_audioEventState == eAudioEventState_Virtual)
				{
					pColor = itemVirtualColor;
				}

				auxGeom.Draw2dLabel(posX, posY, 1.2f,
				                    pColor,
				                    false,
				                    "%s on %s : %u",
				                    szOriginalName,
				                    m_pDebugNameStore->LookupAudioObjectName(pEvent->m_audioObjectId),
				                    pEvent->GetId());

				posY += 10.0f;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventManager::SetDebugNameStore(CATLDebugNameStore const* const pDebugNameStore)
{
	m_pDebugNameStore = pDebugNameStore;
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
