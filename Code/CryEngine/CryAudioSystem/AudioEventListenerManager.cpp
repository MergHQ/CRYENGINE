// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioEventListenerManager.h"

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioEventListenerManager::CAudioEventListenerManager()
{
}

//////////////////////////////////////////////////////////////////////////
CAudioEventListenerManager::~CAudioEventListenerManager()
{
	stl::free_container(m_listeners);
}

//////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioEventListenerManager::AddRequestListener(SAudioManagerRequestDataInternal<eAudioManagerRequestType_AddRequestListener> const* const pRequestData)
{
	EAudioRequestStatus result = eAudioRequestStatus_Failure;

	for (auto const& listener : m_listeners)
	{
		if (listener.OnEvent == pRequestData->func && listener.pObjectToListenTo == pRequestData->pObjectToListenTo
		    && listener.requestType == pRequestData->requestType && listener.specificRequestMask == pRequestData->specificRequestMask)
		{
			result = eAudioRequestStatus_Success;
			break;
		}
	}

	if (result == eAudioRequestStatus_Failure)
	{
		SAudioEventListener audioEventListener;
		audioEventListener.pObjectToListenTo = pRequestData->pObjectToListenTo;
		audioEventListener.OnEvent = pRequestData->func;
		audioEventListener.requestType = pRequestData->requestType;
		audioEventListener.specificRequestMask = pRequestData->specificRequestMask;
		m_listeners.push_back(audioEventListener);
		result = eAudioRequestStatus_Success;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
EAudioRequestStatus CAudioEventListenerManager::RemoveRequestListener(void (* func)(SAudioRequestInfo const* const), void const* const pObjectToListenTo)
{
	EAudioRequestStatus result = eAudioRequestStatus_Failure;
	TListenerArray::iterator Iter(m_listeners.begin());
	TListenerArray::const_iterator const IterEnd(m_listeners.end());

	for (; Iter != IterEnd; ++Iter)
	{
		if ((Iter->OnEvent == func || func == nullptr) && Iter->pObjectToListenTo == pObjectToListenTo)
		{
			if (Iter != IterEnd - 1)
			{
				(*Iter) = m_listeners.back();
			}

			m_listeners.pop_back();
			result = eAudioRequestStatus_Success;

			break;
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (result == eAudioRequestStatus_Failure)
	{
		g_audioLogger.Log(eAudioLogType_Warning, "Failed to remove a request listener!");
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CAudioEventListenerManager::NotifyListener(SAudioRequestInfo const* const pResultInfo)
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);

	for (auto const& listener : m_listeners)
	{
		if (((listener.specificRequestMask & pResultInfo->specificAudioRequest) > 0)                                                  //check: is the listener interested in this specific event?
		    && (listener.pObjectToListenTo == nullptr || listener.pObjectToListenTo == pResultInfo->pOwner)                           //check: is the listener interested in events from this sender
		    && (listener.requestType == eAudioRequestType_AudioAllRequests || listener.requestType == pResultInfo->audioRequestType)) //check: is the listener interested this eventType
		{
			listener.OnEvent(pResultInfo);
		}
	}
}
