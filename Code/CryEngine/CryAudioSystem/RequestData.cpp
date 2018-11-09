// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "RequestData.h"
#include "SystemRequestData.h"
#include "ObjectRequestData.h"
#include "ListenerRequestData.h"
#include "CallbackRequestData.h"

namespace CryAudio
{
#define CRY_AUDIO_REQUEST_CASE_BLOCK(CLASS, ENUM, P_SOURCE, P_RESULT)            \
case ENUM:                                                                       \
	{                                                                              \
		P_RESULT = new CLASS<ENUM>(static_cast<CLASS<ENUM> const* const>(P_SOURCE)); \
                                                                                 \
		break;                                                                       \
	}

#define CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ENUM)   CRY_AUDIO_REQUEST_CASE_BLOCK(SSystemRequestData, ENUM, pRequestData, pResult)
#define CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ENUM) CRY_AUDIO_REQUEST_CASE_BLOCK(SCallbackRequestData, ENUM, pRequestData, pResult)
#define CRY_AUDIO_OBJECT_REQUEST_BLOCK(ENUM)   CRY_AUDIO_REQUEST_CASE_BLOCK(SObjectRequestData, ENUM, pRequestData, pResult)
#define CRY_AUDIO_LISTENER_REQUEST_BLOCK(ENUM) CRY_AUDIO_REQUEST_CASE_BLOCK(SListenerRequestData, ENUM, pRequestData, pResult)

////////////////////////////////////////////////////////////////////////////
SRequestData* AllocateRequestData(SRequestData const* const pRequestData)
{
	CRY_ASSERT(pRequestData != nullptr);
	SRequestData* pResult = nullptr;

	switch (pRequestData->requestType)
	{
	case ERequestType::SystemRequest:
		{
			auto const pBase = static_cast<SSystemRequestDataBase const*>(pRequestData);

			switch (pBase->systemRequestType)
			{
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::SetImpl)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ReleaseImpl)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::RefreshSystem)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::StopAllSounds)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ParseControlsData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ParsePreloadsData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ClearControlsData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ClearPreloadsData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::PreloadSingleRequest)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::UnloadSingleRequest)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::SetParameter)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::SetGlobalParameter)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::SetSwitchState)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::SetGlobalSwitchState)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::AutoLoadSetting)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::LoadSetting)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::UnloadSetting)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::UnloadAFCMDataByScope)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::DrawDebugInfo)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::AddRequestListener)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::RemoveRequestListener)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ChangeLanguage)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::RetriggerControls)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ReleasePendingRays)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ReloadControlsData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::GetFileData)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::GetImplInfo)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::RegisterListener)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ReleaseListener)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::RegisterObject)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ReleaseObject)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ExecuteTrigger)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ExecuteTriggerEx)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ExecuteDefaultTrigger)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ExecutePreviewTrigger)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ExecutePreviewTriggerEx)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::StopTrigger)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::StopAllTriggers)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::StopPreviewTrigger)
				CRY_AUDIO_SYSTEM_REQUEST_BLOCK(ESystemRequestType::ResetRequestCount)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown audio system request type (%u) during %s", pBase->systemRequestType, __FUNCTION__);

					break;
				}
			}

			break;
		}
	case ERequestType::ObjectRequest:
		{
			auto const pBase = static_cast<SObjectRequestDataBase const*>(pRequestData);

			switch (pBase->objectRequestType)
			{
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::LoadTrigger)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::UnloadTrigger)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::PlayFile)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::StopFile)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::ExecuteTrigger)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::StopTrigger)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::StopAllTriggers)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetTransformation)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetOcclusionType)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetOcclusionRayOffset)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetParameter)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetSwitchState)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetCurrentEnvironments)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetEnvironment)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::ProcessPhysicsRay)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::SetName)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::ToggleAbsoluteVelocityTracking)
				CRY_AUDIO_OBJECT_REQUEST_BLOCK(EObjectRequestType::ToggleRelativeVelocityTracking)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown object request type (%u) during %s", pBase->objectRequestType, __FUNCTION__);

					break;
				}
			}
			break;
		}
	case ERequestType::ListenerRequest:
		{
			auto const pBase = static_cast<SListenerRequestDataBase const*>(pRequestData);

			switch (pBase->listenerRequestType)
			{
				CRY_AUDIO_LISTENER_REQUEST_BLOCK(EListenerRequestType::SetTransformation)
				CRY_AUDIO_LISTENER_REQUEST_BLOCK(EListenerRequestType::SetName)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown listener request type (%u) during %s", pBase->listenerRequestType, __FUNCTION__);

					break;
				}
			}

			break;
		}
	case ERequestType::CallbackRequest:
		{
			auto const pBase = static_cast<SCallbackRequestDataBase const*>(pRequestData);

			switch (pBase->callbackRequestType)
			{
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportStartedEvent)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportFinishedEvent)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportFinishedTriggerInstance)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportStartedFile)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportStoppedFile)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportVirtualizedEvent)
				CRY_AUDIO_CALLBACK_REQUEST_BLOCK(ECallbackRequestType::ReportPhysicalizedEvent)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown callback request type (%u) during %s", pBase->callbackRequestType, __FUNCTION__);

					break;
				}
			}

			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(false, "Unknown request type (%u) during %s", pRequestData->requestType, __FUNCTION__);

			break;
		}
	}

	return pResult;
}
} // namespace CryAudio
