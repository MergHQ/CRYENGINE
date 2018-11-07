// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "RequestData.h"
#include "SystemRequestData.h"
#include "ObjectRequestData.h"
#include "ListenerRequestData.h"
#include "CallbackRequestData.h"

namespace CryAudio
{
#define REQUEST_CASE_BLOCK(CLASS, ENUM, P_SOURCE, P_RESULT)                      \
case ENUM:                                                                       \
	{                                                                              \
		P_RESULT = new CLASS<ENUM>(static_cast<CLASS<ENUM> const* const>(P_SOURCE)); \
                                                                                 \
		break;                                                                       \
	}

#define AM_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SSystemRequestData, ENUM, pRequestData, pResult)
#define ACM_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SCallbackRequestData, ENUM, pRequestData, pResult)
#define AO_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SObjectRequestData, ENUM, pRequestData, pResult)
#define AL_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SListenerRequestData, ENUM, pRequestData, pResult)

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
				AM_REQUEST_BLOCK(ESystemRequestType::SetImpl)
				AM_REQUEST_BLOCK(ESystemRequestType::ReleaseImpl)
				AM_REQUEST_BLOCK(ESystemRequestType::RefreshSystem)
				AM_REQUEST_BLOCK(ESystemRequestType::StopAllSounds)
				AM_REQUEST_BLOCK(ESystemRequestType::ParseControlsData)
				AM_REQUEST_BLOCK(ESystemRequestType::ParsePreloadsData)
				AM_REQUEST_BLOCK(ESystemRequestType::ClearControlsData)
				AM_REQUEST_BLOCK(ESystemRequestType::ClearPreloadsData)
				AM_REQUEST_BLOCK(ESystemRequestType::PreloadSingleRequest)
				AM_REQUEST_BLOCK(ESystemRequestType::UnloadSingleRequest)
				AM_REQUEST_BLOCK(ESystemRequestType::SetParameter)
				AM_REQUEST_BLOCK(ESystemRequestType::SetGlobalParameter)
				AM_REQUEST_BLOCK(ESystemRequestType::SetSwitchState)
				AM_REQUEST_BLOCK(ESystemRequestType::SetGlobalSwitchState)
				AM_REQUEST_BLOCK(ESystemRequestType::AutoLoadSetting)
				AM_REQUEST_BLOCK(ESystemRequestType::LoadSetting)
				AM_REQUEST_BLOCK(ESystemRequestType::UnloadSetting)
				AM_REQUEST_BLOCK(ESystemRequestType::UnloadAFCMDataByScope)
				AM_REQUEST_BLOCK(ESystemRequestType::DrawDebugInfo)
				AM_REQUEST_BLOCK(ESystemRequestType::AddRequestListener)
				AM_REQUEST_BLOCK(ESystemRequestType::RemoveRequestListener)
				AM_REQUEST_BLOCK(ESystemRequestType::ChangeLanguage)
				AM_REQUEST_BLOCK(ESystemRequestType::RetriggerControls)
				AM_REQUEST_BLOCK(ESystemRequestType::ReleasePendingRays)
				AM_REQUEST_BLOCK(ESystemRequestType::ReloadControlsData)
				AM_REQUEST_BLOCK(ESystemRequestType::GetFileData)
				AM_REQUEST_BLOCK(ESystemRequestType::GetImplInfo)
				AM_REQUEST_BLOCK(ESystemRequestType::RegisterListener)
				AM_REQUEST_BLOCK(ESystemRequestType::ReleaseListener)
				AM_REQUEST_BLOCK(ESystemRequestType::RegisterObject)
				AM_REQUEST_BLOCK(ESystemRequestType::ReleaseObject)
				AM_REQUEST_BLOCK(ESystemRequestType::ExecuteTrigger)
				AM_REQUEST_BLOCK(ESystemRequestType::ExecuteTriggerEx)
				AM_REQUEST_BLOCK(ESystemRequestType::ExecuteDefaultTrigger)
				AM_REQUEST_BLOCK(ESystemRequestType::ExecutePreviewTrigger)
				AM_REQUEST_BLOCK(ESystemRequestType::ExecutePreviewTriggerEx)
				AM_REQUEST_BLOCK(ESystemRequestType::StopTrigger)
				AM_REQUEST_BLOCK(ESystemRequestType::StopAllTriggers)
				AM_REQUEST_BLOCK(ESystemRequestType::StopPreviewTrigger)
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
				AO_REQUEST_BLOCK(EObjectRequestType::LoadTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::UnloadTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::PlayFile)
				AO_REQUEST_BLOCK(EObjectRequestType::StopFile)
				AO_REQUEST_BLOCK(EObjectRequestType::ExecuteTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::StopTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::StopAllTriggers)
				AO_REQUEST_BLOCK(EObjectRequestType::SetTransformation)
				AO_REQUEST_BLOCK(EObjectRequestType::SetOcclusionType)
				AO_REQUEST_BLOCK(EObjectRequestType::SetOcclusionRayOffset)
				AO_REQUEST_BLOCK(EObjectRequestType::SetParameter)
				AO_REQUEST_BLOCK(EObjectRequestType::SetSwitchState)
				AO_REQUEST_BLOCK(EObjectRequestType::SetCurrentEnvironments)
				AO_REQUEST_BLOCK(EObjectRequestType::SetEnvironment)
				AO_REQUEST_BLOCK(EObjectRequestType::ProcessPhysicsRay)
				AO_REQUEST_BLOCK(EObjectRequestType::SetName)
				AO_REQUEST_BLOCK(EObjectRequestType::ToggleAbsoluteVelocityTracking)
				AO_REQUEST_BLOCK(EObjectRequestType::ToggleRelativeVelocityTracking)
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
				AL_REQUEST_BLOCK(EListenerRequestType::SetTransformation)
				AL_REQUEST_BLOCK(EListenerRequestType::SetName)
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
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportStartedEvent)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportFinishedEvent)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportFinishedTriggerInstance)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportStartedFile)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportStoppedFile)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportVirtualizedEvent)
				ACM_REQUEST_BLOCK(ECallbackRequestType::ReportPhysicalizedEvent)
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
