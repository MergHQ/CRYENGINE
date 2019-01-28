// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "PreviewTrigger.h"

	#include "Common.h"
	#include "Object.h"
	#include "Common/IImpl.h"
	#include "Common/IObject.h"
	#include "Common/ITriggerConnection.h"
	#include "Common/ITriggerInfo.h"
	#include "Common/Logger.h"

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CPreviewTrigger::CPreviewTrigger()
	: Control(g_previewTriggerId, EDataScope::Global, g_szPreviewTriggerName)
	, m_pConnection(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CPreviewTrigger::~CPreviewTrigger()
{
	CRY_ASSERT_MESSAGE(m_pConnection == nullptr, "There is still a connection during %s", __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////
void CPreviewTrigger::Execute(Impl::ITriggerInfo const& triggerInfo)
{
	g_pIImpl->DestructTriggerConnection(m_pConnection);
	m_pConnection = nullptr;
	m_pConnection = g_pIImpl->ConstructTriggerConnection(&triggerInfo);

	if (m_pConnection != nullptr)
	{
		STriggerInstanceState triggerInstanceState;
		triggerInstanceState.triggerId = GetId();

		Impl::IObject* const pIObject = g_previewObject.GetImplDataPtr();

		if (pIObject != nullptr)
		{
			ERequestStatus const activateResult = m_pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

			if (activateResult == ERequestStatus::Success || activateResult == ERequestStatus::Pending)
			{
				if (activateResult == ERequestStatus::Success)
				{
					++(triggerInstanceState.numPlayingInstances);
				}
				else if (activateResult == ERequestStatus::Pending)
				{
					++(triggerInstanceState.numLoadingInstances);
				}
			}
			else if (activateResult != ERequestStatus::SuccessDoNotTrack)
			{
				Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", GetName(), g_previewObject.GetName(), __FUNCTION__);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid impl object during %s", __FUNCTION__);
		}

		if (triggerInstanceState.numPlayingInstances > 0 || triggerInstanceState.numLoadingInstances > 0)
		{
			triggerInstanceState.flags |= ETriggerStatus::Playing;
			g_triggerInstanceIdToObject[g_triggerInstanceIdCounter] = &g_previewObject;
			g_previewObject.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
			IncrementTriggerInstanceIdCounter();
		}
		else
		{
			// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
			g_previewObject.SendFinishedTriggerInstanceRequest(triggerInstanceState);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPreviewTrigger::Clear()
{
	g_pIImpl->DestructTriggerConnection(m_pConnection);
	m_pConnection = nullptr;
}
}      // namespace CryAudio
#endif // CRY_AUDIO_USE_PRODUCTION_CODE