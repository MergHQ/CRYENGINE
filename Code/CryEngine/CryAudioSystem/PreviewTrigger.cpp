// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "PreviewTrigger.h"

	#include "Common.h"
	#include "GlobalObject.h"
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
		STriggerInstanceState triggerInstanceState(m_id);

		Impl::IObject* const pIObject = g_previewObject.GetImplDataPtr();
		ETriggerResult const result = m_pConnection->Execute(pIObject, g_triggerInstanceIdCounter);

		if ((result == ETriggerResult::Playing) || (result == ETriggerResult::Virtual) || (result == ETriggerResult::Pending))
		{
			if (result != ETriggerResult::Pending)
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
			Cry::Audio::Log(ELogType::Warning, R"(Trigger "%s" failed on object "%s" during %s)", GetName(), g_previewObject.GetName(), __FUNCTION__);
		}

		if ((triggerInstanceState.numPlayingInstances > 0) || (triggerInstanceState.numPendingInstances > 0))
		{
			triggerInstanceState.flags |= ETriggerStatus::Playing;
			g_triggerInstanceIdToGlobalObject[g_triggerInstanceIdCounter] = &g_previewObject;
			g_previewObject.AddTriggerState(g_triggerInstanceIdCounter, triggerInstanceState);
			IncrementTriggerInstanceIdCounter();
		}
		else
		{
			// All of the events have either finished before we got here or never started, immediately inform the user that the trigger has finished.
			SendFinishedTriggerInstanceRequest(triggerInstanceState, INVALID_ENTITYID);
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