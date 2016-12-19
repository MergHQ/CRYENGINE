// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioEventListenerManager.h"

using namespace CryAudio;
using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioEventListenerManager::~CAudioEventListenerManager()
{
	stl::free_container(m_listeners);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioEventListenerManager::AddRequestListener(SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> const* const pRequestData)
{
	ERequestStatus result = eRequestStatus_Failure;

	for (auto const& listener : m_listeners)
	{
		if (listener.OnEvent == pRequestData->func && listener.pObjectToListenTo == pRequestData->pObjectToListenTo
		    && listener.eventMask == pRequestData->eventMask)
		{
			result = eRequestStatus_Success;
			break;
		}
	}

	if (result == eRequestStatus_Failure)
	{
		SAudioEventListener audioEventListener;
		audioEventListener.pObjectToListenTo = pRequestData->pObjectToListenTo;
		audioEventListener.OnEvent = pRequestData->func;
		audioEventListener.eventMask = pRequestData->eventMask;
		m_listeners.push_back(audioEventListener);
		result = eRequestStatus_Success;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioEventListenerManager::RemoveRequestListener(void (* func)(SRequestInfo const* const), void const* const pObjectToListenTo)
{
	ERequestStatus result = eRequestStatus_Failure;
	ListenerArray::iterator Iter(m_listeners.begin());
	ListenerArray::const_iterator const IterEnd(m_listeners.end());

	for (; Iter != IterEnd; ++Iter)
	{
		if ((Iter->OnEvent == func || func == nullptr) && Iter->pObjectToListenTo == pObjectToListenTo)
		{
			if (Iter != IterEnd - 1)
			{
				(*Iter) = m_listeners.back();
			}

			m_listeners.pop_back();
			result = eRequestStatus_Success;

			break;
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (result == eRequestStatus_Failure)
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to remove a request listener!");
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventListenerManager::NotifyListener(SRequestInfo const* const pResultInfo)
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);

	for (auto const& listener : m_listeners)
	{
		if (((listener.eventMask & pResultInfo->audioSystemEvent) > 0)                                           //check: is the listener interested in this specific event?
		    && (listener.pObjectToListenTo == nullptr || listener.pObjectToListenTo == pResultInfo->pOwner))     //check: is the listener interested in events from this sender
		{
			listener.OnEvent(pResultInfo);
		}
	}
}
