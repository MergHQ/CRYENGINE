// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "DefaultTriggerUtils.h"
#include "Managers.h"
#include "EventManager.h"
#include "Object.h"
#include "Event.h"
#include "TriggerConnection.h"
#include "Common/IEvent.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include "Common/ISetting.h"
#include "Common/ITrigger.h"
#include "Common/Logger.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void ExecuteDefaultTriggerConnections(Control const* const pControl, TriggerConnections const& connections)
{
	STriggerInstanceState triggerInstanceState;
	triggerInstanceState.triggerId = pControl->GetId();

	for (auto const pConnection : connections)
	{
		CEvent* const pEvent = g_eventManager.ConstructEvent();
		ERequestStatus const activateResult = pConnection->Execute(g_pObject->GetImplDataPtr(), pEvent->m_pImplData);

		if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual) || (activateResult == ERequestStatus::Pending))
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pEvent->SetTriggerName(pControl->GetName());
			CRY_ASSERT_MESSAGE(pControl->GetDataScope() == EDataScope::Global, "Default controls must always have global data scope! (%s) during %s", pControl->GetName(), __FUNCTION__);
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			pEvent->m_pObject = g_pObject;
			pEvent->SetTriggerId(pControl->GetId());
			pEvent->m_triggerImplId = pConnection->m_triggerImplId;
			pEvent->m_triggerInstanceId = g_triggerInstanceIdCounter;

			if ((activateResult == ERequestStatus::Success) || (activateResult == ERequestStatus::SuccessVirtual))
			{
				pEvent->m_state = EEventState::Playing;
				++(triggerInstanceState.numPlayingEvents);
			}
			else if (activateResult == ERequestStatus::Pending)
			{
				pEvent->m_state = EEventState::Loading;
				++(triggerInstanceState.numLoadingEvents);
			}

			g_pObject->AddEvent(pEvent);
		}
		else
		{
			g_eventManager.DestructEvent(pEvent);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				// No TriggerImpl generated an active event.
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s")", pControl->GetName(), g_pObject->m_name.c_str());
			}
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
		}
	}

	if (triggerInstanceState.numPlayingEvents > 0 || triggerInstanceState.numLoadingEvents > 0)
	{
		triggerInstanceState.flags |= ETriggerStatus::Playing;
		g_pObject->AddTriggerState(g_triggerInstanceIdCounter++, triggerInstanceState);
	}
	else
	{
		// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
		g_pObject->SendFinishedTriggerInstanceRequest(triggerInstanceState);
	}
}
} // namespace CryAudio
