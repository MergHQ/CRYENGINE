// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "DefaultTriggerUtils.h"
#include "Object.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/ITriggerConnection.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Common/Logger.h"
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections)
{
	STriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = pControl->GetId();

	Impl::IObject* const pIObject = g_object.GetImplDataPtr();

	if (pIObject != nullptr)
	{
		for (auto const pConnection : connections)
		{
			ERequestStatus const activateResult = pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

			if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual) || (activateResult == ERequestStatus::Pending))
			{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s) during %s", pControl->GetName(), __FUNCTION__);
#endif    // INCLUDE_AUDIO_PRODUCTION_CODE

				if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual))
				{
					++(triggerInstanceState.numPlayingInstances);
				}
				else if (activateResult == ERequestStatus::Pending)
				{
					++(triggerInstanceState.numLoadingInstances);
				}
			}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			else if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", pControl->GetName(), g_object.GetName(), __FUNCTION__);
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
	}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

	if (triggerInstanceState.numPlayingInstances > 0 || triggerInstanceState.numLoadingInstances > 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_triggerInstanceIdToObject[g_triggerInstanceIdCounter] = &g_object;
		g_object.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
		IncrementTriggerInstanceIdCounter();
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		g_object.SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}
}
} // namespace CryAudio
