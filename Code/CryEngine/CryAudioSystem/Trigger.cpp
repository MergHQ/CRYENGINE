// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Managers.h"
#include "FileManager.h"
#include "Object.h"
#include "Trigger.h"
#include "StandaloneFile.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/IStandaloneFileConnection.h"
#include "Common/ITriggerConnection.h"

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Common/Logger.h"
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

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

	bool isPlaying = false;

	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
		for (auto const pConnection : m_connections)
		{
			ETriggerResult const result = pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

			if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual))
			{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				triggerInstanceState.radius = m_radius;
				object.UpdateMaxRadius(m_radius);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				++(triggerInstanceState.numPlayingInstances);

				if (result == ETriggerResult::Playing)
				{
					isPlaying = true;
				}
			}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			else if (result != ETriggerResult::DoNotTrack)
			{
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", GetName(), object.GetName(), __FUNCTION__);
			}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

	if (triggerInstanceState.numPlayingInstances > 0)
	{
		if (isPlaying)
		{
			object.RemoveFlag(EObjectFlags::Virtual);
		}
		else if ((object.GetFlags() & EObjectFlags::Active) == 0)
		{
			// Only when no trigger was active before, the object can get set to virtual here.
			object.SetFlag(EObjectFlags::Virtual);
		}

		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_triggerInstanceIdToObject[g_triggerInstanceIdCounter] = &object;
		object.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
		IncrementTriggerInstanceIdCounter();
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		object.SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.GetName());
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(Impl::IObject* const pIObject) const
{
	if (pIObject != nullptr)
	{
		for (auto const pConnection : m_connections)
		{
			pConnection->Stop(pIObject);
		}
	}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
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
	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
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

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				pFile->m_triggerId = GetId();
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				object.AddStandaloneFile(pFile, SUserDataBase(pOwner, pUserData, pUserDataOwner));
			}
			else
			{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				Cry::Audio::Log(ELogType::Warning, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.GetName());
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				g_fileManager.ReleaseStandaloneFile(pFile);
			}
		}
	}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CObject& object,
	TriggerInstanceId const triggerInstanceId,
	STriggerInstanceState& triggerInstanceState,
	uint16 const triggerCounter) const
{
	bool isPlaying = false;

	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
		for (auto const pConnection : m_connections)
		{
			ETriggerResult const result = pConnection->Execute(pIObject, triggerInstanceId);

			if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual))
			{
				triggerInstanceState.radius = m_radius;
				object.UpdateMaxRadius(m_radius);

				++(triggerInstanceState.numPlayingInstances);

				if (result == ETriggerResult::Playing)
				{
					isPlaying = true;
				}
			}
			else if (result != ETriggerResult::DoNotTrack)
			{
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", GetName(), object.GetName(), __FUNCTION__);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}

	if (isPlaying)
	{
		object.RemoveFlag(EObjectFlags::Virtual);
	}
	else if (triggerCounter == 0)
	{
		// If the first trigger is not virtual, the object cannot get set to virtual here afterwards.
		object.SetFlag(EObjectFlags::Virtual);
	}

	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.GetName());
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::PlayFile(CObject& object, CStandaloneFile* const pFile) const
{
	CRY_ASSERT_MESSAGE(pFile->m_pImplData == nullptr, "Standalone file impl data survided a middleware switch during %s", __FUNCTION__);

	Impl::IObject* const pIObject = object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
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
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				Cry::Audio::Log(ELogType::Error, R"(PlayFile failed with "%s" on object "%s")", pFile->m_hashedFilename.GetText().c_str(), object.GetName());
	#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

				g_pIImpl->DestructStandaloneFileConnection(pFile->m_pImplData);
				pFile->m_pImplData = nullptr;
			}
		}
	}
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE

}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}      // namespace CryAudio
