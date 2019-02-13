// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Managers.h"
#include "Object.h"
#include "GlobalObject.h"
#include "TriggerUtils.h"
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
	STriggerInstanceState triggerInstanceState(m_id, pOwner, pUserData, pUserDataOwner);

	if ((flags& ERequestFlags::DoneCallbackOnAudioThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnAudioThread;
	}
	else if ((flags& ERequestFlags::DoneCallbackOnExternalThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnExternalThread;
	}

	bool const isPlaying = ExecuteConnections(object, g_triggerInstanceIdCounter, triggerInstanceState);

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
		SendFinishedTriggerInstanceRequest(triggerInstanceState, object.GetEntityId());
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), object.GetName());
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CGlobalObject& globalObject,
	void* const pOwner /* = nullptr */,
	void* const pUserData /* = nullptr */,
	void* const pUserDataOwner /* = nullptr */,
	ERequestFlags const flags /* = ERequestFlags::None */) const
{
	STriggerInstanceState triggerInstanceState(m_id, pOwner, pUserData, pUserDataOwner);

	if ((flags& ERequestFlags::DoneCallbackOnAudioThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnAudioThread;
	}
	else if ((flags& ERequestFlags::DoneCallbackOnExternalThread) != 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::CallbackOnExternalThread;
	}

	ExecuteConnections(globalObject, g_triggerInstanceIdCounter, triggerInstanceState);

	if ((triggerInstanceState.numPlayingInstances > 0) || (triggerInstanceState.numPendingInstances > 0))
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_triggerInstanceIdToGlobalObject[g_triggerInstanceIdCounter] = &globalObject;
		globalObject.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
		IncrementTriggerInstanceIdCounter();
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		SendFinishedTriggerInstanceRequest(triggerInstanceState, INVALID_ENTITYID);
	}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), globalObject.GetName());
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
bool CTrigger::ExecuteConnections(CObject& object, TriggerInstanceId const triggerInstanceId, STriggerInstanceState& triggerInstanceState) const
{
	bool isPlaying = false;

	Impl::IObject* const pIObject = object.GetImplDataPtr();

	for (auto const pConnection : m_connections)
	{
		ETriggerResult const result = pConnection->Execute(pIObject, triggerInstanceId);

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

	return isPlaying;
}

//////////////////////////////////////////////////////////////////////////
bool CTrigger::ExecuteConnections(CGlobalObject& globalObject, TriggerInstanceId const triggerInstanceId, STriggerInstanceState& triggerInstanceState) const
{
	bool isPlaying = false;

	Impl::IObject* const pIObject = globalObject.GetImplDataPtr();

	for (auto const pConnection : m_connections)
	{
		ETriggerResult const result = pConnection->Execute(pIObject, triggerInstanceId);

		if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
		{
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
			Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", GetName(), globalObject.GetName(), __FUNCTION__);
		}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
	}

	return isPlaying;
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(Impl::IObject* const pIObject) const
{
	for (auto const pConnection : m_connections)
	{
		pConnection->Stop(pIObject);
	}
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CTrigger::Execute(
	CObject& object,
	TriggerInstanceId const triggerInstanceId,
	STriggerInstanceState& triggerInstanceState,
	uint16 const triggerCounter) const
{
	if (ExecuteConnections(object, triggerInstanceId, triggerInstanceState))
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
void CTrigger::Execute(
	CGlobalObject& globalObject,
	TriggerInstanceId const triggerInstanceId,
	STriggerInstanceState& triggerInstanceState,
	uint16 const triggerCounter) const
{
	ExecuteConnections(globalObject, triggerInstanceId, triggerInstanceState);

	if (m_connections.empty())
	{
		Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" executed on object "%s" without connections)", GetName(), globalObject.GetName());
	}
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}      // namespace CryAudio
