// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Managers.h"
#include "Object.h"
#include "Trigger.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
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

			if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
			{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				triggerInstanceState.radius = m_radius;
				object.UpdateMaxRadius(m_radius);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				if (result == ETriggerResult::Playing)
				{
					++(triggerInstanceState.numPlayingInstances);
					isPlaying = true;
				}
				else if (result == ETriggerResult::Virtual)
				{
					++(triggerInstanceState.numPlayingInstances);
				}
				else
				{
					++(triggerInstanceState.numPendingInstances);
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

	if ((triggerInstanceState.numPlayingInstances > 0) || (triggerInstanceState.numPendingInstances > 0))
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

			if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
			{
				triggerInstanceState.radius = m_radius;
				object.UpdateMaxRadius(m_radius);

				if (result == ETriggerResult::Playing)
				{
					++(triggerInstanceState.numPlayingInstances);
					isPlaying = true;
				}
				else if (result == ETriggerResult::Virtual)
				{
					++(triggerInstanceState.numPlayingInstances);
				}
				else
				{
					++(triggerInstanceState.numPendingInstances);
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
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}      // namespace CryAudio
