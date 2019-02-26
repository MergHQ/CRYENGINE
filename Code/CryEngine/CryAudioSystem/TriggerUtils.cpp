// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "TriggerUtils.h"
#include "GlobalObject.h"
#include "System.h"
#include "CallbackRequestData.h"
#include "Common/IObject.h"
#include "Common/ITriggerConnection.h"

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	#include "Common/Logger.h"
#endif  // CRY_AUDIO_USE_DEBUG_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void SendFinishedTriggerInstanceRequest(
	ControlId const triggerId,
	EntityId const entityId,
	ERequestFlags const flags /*= ERequestFlags::None*/,
	void* const pOwner /*= nullptr*/,
	void* const pUserData /*= nullptr*/,
	void* const pUserDataOwner /*= nullptr*/)
{
	if ((flags& ERequestFlags::DoneCallbackOnExternalThread) != 0)
	{
		SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const requestData(triggerId, entityId);
		CRequest const request(&requestData, ERequestFlags::CallbackOnExternalOrCallingThread, pOwner, pUserData, pUserDataOwner);
		g_system.PushRequest(request);
	}
	else if ((flags& ERequestFlags::DoneCallbackOnAudioThread) != 0)
	{
		SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const requestData(triggerId, entityId);
		CRequest const request(&requestData, ERequestFlags::CallbackOnAudioThread, pOwner, pUserData, pUserDataOwner);
		g_system.PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections)
{
	Impl::IObject* const pIObject = g_object.GetImplDataPtr();

	uint16 numPlayingInstances = 0;
	uint16 numPendingInstances = 0;

	for (auto const pConnection : connections)
	{
		ETriggerResult const result = pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

		if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
		{
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
			CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s) during %s", pControl->GetName(), __FUNCTION__);
#endif    // CRY_AUDIO_USE_DEBUG_CODE

			if (result != ETriggerResult::Pending)
			{
				++numPlayingInstances;
			}
			else
			{
				++numPendingInstances;
			}
		}
#if defined(CRY_AUDIO_USE_DEBUG_CODE)
		else if (result != ETriggerResult::DoNotTrack)
		{
			Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", pControl->GetName(), g_object.GetName(), __FUNCTION__);
		}
#endif  // CRY_AUDIO_USE_DEBUG_CODE
	}

	if ((numPlayingInstances > 0) || (numPendingInstances > 0))
	{
		g_object.ConstructTriggerInstance(pControl->GetId(), numPlayingInstances, numPendingInstances, ERequestFlags::None, nullptr, nullptr, nullptr);
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		SendFinishedTriggerInstanceRequest(pControl->GetId(), INVALID_ENTITYID);
	}
}
} // namespace CryAudio
