// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "TriggerUtils.h"
#include "GlobalObject.h"
#include "System.h"
#include "CallbackRequestData.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/ITriggerConnection.h"

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Common/Logger.h"
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void SendFinishedTriggerInstanceRequest(STriggerInstanceState const& triggerInstanceState, EntityId const entityId)
{
	if ((triggerInstanceState.flags & ETriggerStatus::CallbackOnExternalThread) != 0)
	{
		SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const requestData(triggerInstanceState.triggerId, entityId);
		CRequest const request(&requestData, ERequestFlags::CallbackOnExternalOrCallingThread, triggerInstanceState.pOwnerOverride, triggerInstanceState.pUserData, triggerInstanceState.pUserDataOwner);
		g_system.PushRequest(request);
	}
	else if ((triggerInstanceState.flags & ETriggerStatus::CallbackOnAudioThread) != 0)
	{
		SCallbackRequestData<ECallbackRequestType::ReportFinishedTriggerInstance> const requestData(triggerInstanceState.triggerId, entityId);
		CRequest const request(&requestData, ERequestFlags::CallbackOnAudioThread, triggerInstanceState.pOwnerOverride, triggerInstanceState.pUserData, triggerInstanceState.pUserDataOwner);
		g_system.PushRequest(request);
	}
}

//////////////////////////////////////////////////////////////////////////
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections)
{
	STriggerInstanceState triggerInstanceState(pControl->GetId());

	Impl::IObject* const pIObject = g_object.GetImplDataPtr();

	for (auto const pConnection : connections)
	{
		ETriggerResult const result = pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

		if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s) during %s", pControl->GetName(), __FUNCTION__);
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

			if (result != ETriggerResult::Pending)
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
			Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", pControl->GetName(), g_object.GetName(), __FUNCTION__);
		}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
	}

	if ((triggerInstanceState.numPlayingInstances > 0) || (triggerInstanceState.numPendingInstances > 0))
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_triggerInstanceIdToGlobalObject[g_triggerInstanceIdCounter] = &g_object;
		g_object.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
		IncrementTriggerInstanceIdCounter();
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		SendFinishedTriggerInstanceRequest(triggerInstanceState, INVALID_ENTITYID);
	}
}
} // namespace CryAudio
