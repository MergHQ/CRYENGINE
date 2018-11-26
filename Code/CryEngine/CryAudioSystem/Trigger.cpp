// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Managers.h"
#include "EventManager.h"
#include "FileManager.h"
#include "Object.h"
#include "Event.h"
#include "Trigger.h"
#include "StandaloneFile.h"
#include "Common/IEvent.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/IStandaloneFileConnection.h"
#include "Common/ITriggerConnection.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Common/Logger.h"
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CTrigger::~CTrigger()
{
	CRY_ASSERT_MESSAGE(g_pIImpl != nullptr, "g_pIImpl mustn't be nullptr during %s", __FUNCTION__);

	for (auto const pConnection : m_connections)
	{
		g_pIImpl->DestructTriggerConnection(pConnection);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CObject& object,
	void* const pOwner /* = nullptr */,
	void* const pUserData /* = nullptr */,
	void* const pUserDataOwner /* = nullptr */,
	ERequestFlags const flags /* = ERequestFlags::None */) const
{
	object.UpdateOcclusion();

	STriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = GetId();
	triggerInstanceState.pOwnerOverride = pOwner;
	triggerInstanceState.pUserData = pUserData;
	triggerInstanceState.pUserDataOwner = pUserDataOwner;

	if ((flags& ERequestFlags::DoneCallbackOnAudioThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnAudioThread;
	}
	else if ((flags& ERequestFlags::DoneCallbackOnExternalThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnExternalThread;
	}

	for (auto const pConnection : m_connections)
	{
		CEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(object.GetImplDataPtr(), pEvent->m_pImplData);

		if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual) || (activateResult == ERequestStatus::Pending))
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(GetName());
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->m_triggerInstanceId = g_triggerInstanceIdCounter;

			if (activateResult == ERequestStatus::Success)
			{
				pEvent->SetPlaying();
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::SuccessVirtual)
			{
				pEvent->SetVirtual();
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", GetName(), object.m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	if (triggerInstanceState.numPlayingEvents > 0 || triggerInstanceState.numLoadingEvents > 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		object.AddTriggerState(g_triggerInstanceIdCounter++, triggerInstanceState);
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		object.SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CObject& object,
	TriggerInstanceId const triggerInstanceId,
	STriggerInstanceState& triggerInstanceState) const
{
	object.UpdateOcclusion();

	for (auto const pConnection : m_connections)
	{
		CEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(object.GetImplDataPtr(), pEvent->m_pImplData);

		if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual) || (activateResult == ERequestStatus::Pending))
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(GetName());
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->m_triggerInstanceId = triggerInstanceId;

			if (activateResult == ERequestStatus::Success)
			{
				pEvent->SetPlaying();
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::SuccessVirtual)
			{
				pEvent->SetVirtual();
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", GetName(), object.m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.m_name.c_str());
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::LoadAsync(CObject& object, bool const doLoad) const
{
	// TODO: This needs proper implementation!
	STriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = GetId();

	for (auto const pConnection : m_connections)
	{
		CEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus prepUnprepResult = ERequestStatus::Failure;

		if (doLoad)
		{
			if (((triggerInstanceState.flags & ETriggerStatus::Loaded) == 0) && ((triggerInstanceState.flags & ETriggerStatus::Loading) == 0))
			{
				prepUnprepResult = pConnection->LoadAsync(pEvent->m_pImplData);
			}
		}
		else
		{
			if (((triggerInstanceState.flags & ETriggerStatus::Loaded) != 0) && ((triggerInstanceState.flags & ETriggerStatus::Unloading) == 0))
			{
				prepUnprepResult = pConnection->UnloadAsync(pEvent->m_pImplData);
			}
		}

		if (prepUnprepResult == ERequestStatus::Success)
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerRadius(m_radius);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pObject = &object;
			pEvent->SetTriggerId(GetId());
			pEvent->m_triggerInstanceId = g_triggerInstanceIdCounter;
			pEvent->m_state = doLoad ? EEventState::Loading : EEventState::Unloading;

			object.AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(LoadAsync failed on trigger "%s" for object "%s")", GetName(), object.m_name.c_str());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	object.AddTriggerState(g_triggerInstanceIdCounter++, triggerInstanceState);
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::PlayFile(
	CObject& object,
	char const* const szName,
	bool const isLocalized,
	void* const pOwner /* = nullptr */,
	void* const pUserData /* = nullptr */,
	void* const pUserDataOwner /* = nullptr */) const
{
	if (!m_connections.empty())
	{
		Impl::ITriggerConnection const* const pITriggerConnection = m_connections[0];
		CStandaloneFile* const pFile = g_fileManager.ConstructStandaloneFile(szName, isLocalized, pITriggerConnection);
		ERequestStatus const status = pFile->m_pImplData->Play(object.GetImplDataPtr());

		if (status == ERequestStatus::Success || status == ERequestStatus::Pending)
		{
			if (status == ERequestStatus::Success)
			{
				pFile->m_state = EStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EStandaloneFileState::Loading;
			}

			pFile->m_pObject = &object;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pFile->m_triggerId = GetId();
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			object.AddStandaloneFile(pFile, SUserDataBase(pOwner, pUserData, pUserDataOwner));
		}
		else
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Warning, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.m_name.c_str());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			g_fileManager.ReleaseStandaloneFile(pFile);
		}
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CTrigger::PlayFile(CObject& object, CStandaloneFile* const pFile) const
{
	CRY_ASSERT_MESSAGE(pFile->m_pImplData == nullptr, "Standalone file impl data survided a middleware switch during %s", __FUNCTION__);

	if (!m_connections.empty())
	{
		Impl::ITriggerConnection const* const pITriggerConnection = m_connections[0];
		pFile->m_pImplData = g_pIImpl->ConstructStandaloneFileConnection(*pFile, pFile->m_hashedFilename.GetText().c_str(), pFile->m_isLocalized, pITriggerConnection);
		ERequestStatus const status = pFile->m_pImplData->Play(object.GetImplDataPtr());

		if (status == ERequestStatus::Success || status == ERequestStatus::Pending)
		{
			if (status == ERequestStatus::Success)
			{
				pFile->m_state = EStandaloneFileState::Playing;
			}
			else if (status == ERequestStatus::Pending)
			{
				pFile->m_state = EStandaloneFileState::Loading;
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.m_name.c_str());

			g_pIImpl->DestructStandaloneFileConnection(pFile->m_pImplData);
			pFile->m_pImplData = nullptr;
		}
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
