// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioInternalInterfaces.h"

namespace CryAudio
{
#define REQUEST_CASE_BLOCK(CLASS, ENUM, P_SOURCE, P_RESULT)                      \
case ENUM:                                                                       \
	{                                                                              \
		P_RESULT = new CLASS<ENUM>(static_cast<CLASS<ENUM> const* const>(P_SOURCE)); \
                                                                                 \
		break;                                                                       \
	}

#define AM_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SManagerRequestData, ENUM, pRequestData, pResult)
#define ACM_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SCallbackManagerRequestData, ENUM, pRequestData, pResult)
#define AO_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SObjectRequestData, ENUM, pRequestData, pResult)
#define AL_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SListenerRequestData, ENUM, pRequestData, pResult)

////////////////////////////////////////////////////////////////////////////
SRequestData* AllocateRequestData(SRequestData const* const pRequestData)
{
	CRY_ASSERT(pRequestData != nullptr);
	SRequestData* pResult = nullptr;

	switch (pRequestData->requestType)
	{
	case ERequestType::ManagerRequest:
		{
			SManagerRequestDataBase const* const pBase = static_cast<SManagerRequestDataBase const* const>(pRequestData);

			switch (pBase->managerRequestType)
			{
				AM_REQUEST_BLOCK(EManagerRequestType::SetAudioImpl)
				AM_REQUEST_BLOCK(EManagerRequestType::ReleaseAudioImpl)
				AM_REQUEST_BLOCK(EManagerRequestType::RefreshAudioSystem)
				AM_REQUEST_BLOCK(EManagerRequestType::StopAllSounds)
				AM_REQUEST_BLOCK(EManagerRequestType::ParseControlsData)
				AM_REQUEST_BLOCK(EManagerRequestType::ParsePreloadsData)
				AM_REQUEST_BLOCK(EManagerRequestType::ClearControlsData)
				AM_REQUEST_BLOCK(EManagerRequestType::ClearPreloadsData)
				AM_REQUEST_BLOCK(EManagerRequestType::PreloadSingleRequest)
				AM_REQUEST_BLOCK(EManagerRequestType::UnloadSingleRequest)
				AM_REQUEST_BLOCK(EManagerRequestType::AutoLoadSetting)
				AM_REQUEST_BLOCK(EManagerRequestType::LoadSetting)
				AM_REQUEST_BLOCK(EManagerRequestType::UnloadSetting)
				AM_REQUEST_BLOCK(EManagerRequestType::UnloadAFCMDataByScope)
				AM_REQUEST_BLOCK(EManagerRequestType::DrawDebugInfo)
				AM_REQUEST_BLOCK(EManagerRequestType::AddRequestListener)
				AM_REQUEST_BLOCK(EManagerRequestType::RemoveRequestListener)
				AM_REQUEST_BLOCK(EManagerRequestType::ChangeLanguage)
				AM_REQUEST_BLOCK(EManagerRequestType::RetriggerAudioControls)
				AM_REQUEST_BLOCK(EManagerRequestType::ReleasePendingRays)
				AM_REQUEST_BLOCK(EManagerRequestType::ReloadControlsData)
				AM_REQUEST_BLOCK(EManagerRequestType::GetAudioFileData)
				AM_REQUEST_BLOCK(EManagerRequestType::GetImplInfo)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown audio manager request type (%u)", pBase->managerRequestType);

					break;
				}
			}

			break;
		}
	case ERequestType::ObjectRequest:
		{
			SObjectRequestDataBase const* const pBase = static_cast<SObjectRequestDataBase const* const>(pRequestData);

			switch (pBase->objectRequestType)
			{
				AO_REQUEST_BLOCK(EObjectRequestType::LoadTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::UnloadTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::PlayFile)
				AO_REQUEST_BLOCK(EObjectRequestType::StopFile)
				AO_REQUEST_BLOCK(EObjectRequestType::ExecuteTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::ExecuteTriggerEx)
				AO_REQUEST_BLOCK(EObjectRequestType::StopTrigger)
				AO_REQUEST_BLOCK(EObjectRequestType::StopAllTriggers)
				AO_REQUEST_BLOCK(EObjectRequestType::SetTransformation)
				AO_REQUEST_BLOCK(EObjectRequestType::SetOcclusionType)
				AO_REQUEST_BLOCK(EObjectRequestType::SetParameter)
				AO_REQUEST_BLOCK(EObjectRequestType::SetSwitchState)
				AO_REQUEST_BLOCK(EObjectRequestType::SetCurrentEnvironments)
				AO_REQUEST_BLOCK(EObjectRequestType::SetEnvironment)
				AO_REQUEST_BLOCK(EObjectRequestType::RegisterObject)
				AO_REQUEST_BLOCK(EObjectRequestType::ReleaseObject)
				AO_REQUEST_BLOCK(EObjectRequestType::ProcessPhysicsRay)
				AO_REQUEST_BLOCK(EObjectRequestType::SetName)
				AO_REQUEST_BLOCK(EObjectRequestType::ToggleAbsoluteVelocityTracking)
				AO_REQUEST_BLOCK(EObjectRequestType::ToggleRelativeVelocityTracking)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown object request type (%u)", pBase->objectRequestType);

					break;
				}
			}
			break;
		}
	case ERequestType::ListenerRequest:
		{
			SListenerRequestDataBase const* const pBase = static_cast<SListenerRequestDataBase const* const>(pRequestData);

			switch (pBase->listenerRequestType)
			{
				AL_REQUEST_BLOCK(EListenerRequestType::SetTransformation)
				AL_REQUEST_BLOCK(EListenerRequestType::RegisterListener)
				AL_REQUEST_BLOCK(EListenerRequestType::ReleaseListener)
				AL_REQUEST_BLOCK(EListenerRequestType::SetName)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown listener request type (%u)", pBase->listenerRequestType);

					break;
				}
			}

			break;
		}
	case ERequestType::CallbackManagerRequest:
		{
			SCallbackManagerRequestDataBase const* const pBase = static_cast<SCallbackManagerRequestDataBase const* const>(pRequestData);

			switch (pBase->callbackManagerRequestType)
			{
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportStartedEvent)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportFinishedEvent)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportFinishedTriggerInstance)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportStartedFile)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportStoppedFile)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportVirtualizedEvent)
				ACM_REQUEST_BLOCK(ECallbackManagerRequestType::ReportPhysicalizedEvent)
			default:
				{
					CRY_ASSERT_MESSAGE(false, "Unknown callback manager request type (%u)", pBase->callbackManagerRequestType);

					break;
				}
			}

			break;
		}
	default:
		{
			CRY_ASSERT_MESSAGE(false, "Unknown request type (%u)", pRequestData->requestType);

			break;
		}
	}

	return pResult;
}
} // namespace CryAudio
