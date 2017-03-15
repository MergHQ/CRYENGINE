// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioInternalInterfaces.h"

using namespace CryAudio;

#define REQUEST_CASE_BLOCK(CLASS, ENUM, P_SOURCE, P_RESULT)                        \
  case ENUM:                                                                       \
    {                                                                              \
      P_RESULT = new CLASS<ENUM>(static_cast<CLASS<ENUM> const* const>(P_SOURCE)); \
                                                                                   \
      break;                                                                       \
    }

#define AM_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SAudioManagerRequestData, ENUM, pRequestData, pResult)
#define ACM_REQUEST_BLOCK(ENUM) REQUEST_CASE_BLOCK(SAudioCallbackManagerRequestData, ENUM, pRequestData, pResult)
#define AO_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SAudioObjectRequestData, ENUM, pRequestData, pResult)
#define AL_REQUEST_BLOCK(ENUM)  REQUEST_CASE_BLOCK(SAudioListenerRequestData, ENUM, pRequestData, pResult)

////////////////////////////////////////////////////////////////////////////
SAudioRequestData* CryAudio::AllocateRequestData(SAudioRequestData const* const pRequestData)
{
	CRY_ASSERT(pRequestData != nullptr);
	SAudioRequestData* pResult = nullptr;
	EAudioRequestType const requestType = pRequestData->type;

	switch (requestType)
	{
	case eAudioRequestType_AudioManagerRequest:
		{
			SAudioManagerRequestDataBase const* const pBase = static_cast<SAudioManagerRequestDataBase const* const>(pRequestData);

			switch (pBase->type)
			{
				AM_REQUEST_BLOCK(eAudioManagerRequestType_SetAudioImpl)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ReleaseAudioImpl)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_RefreshAudioSystem)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ConstructAudioListener)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_LoseFocus)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_GetFocus)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_MuteAll)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_UnmuteAll)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_StopAllSounds)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ParseControlsData)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ParsePreloadsData)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ClearControlsData)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ClearPreloadsData)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_PreloadSingleRequest)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_UnloadSingleRequest)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_UnloadAFCMDataByScope)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_DrawDebugInfo)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_AddRequestListener)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_RemoveRequestListener)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ChangeLanguage)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_RetriggerAudioControls)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ReleasePendingRays)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_ReloadControlsData)
				AM_REQUEST_BLOCK(eAudioManagerRequestType_GetAudioFileData)
			default:
				{
					g_audioLogger.Log(eAudioLogType_Error, "Unknown audio manager request type (%u)", pBase->type);
					CRY_ASSERT(false);

					break;
				}
			}

			break;
		}
	case eAudioRequestType_AudioObjectRequest:
		{
			SAudioObjectRequestDataBase const* const pBase = static_cast<SAudioObjectRequestDataBase const* const>(pRequestData);

			switch (pBase->type)
			{
				AO_REQUEST_BLOCK(eAudioObjectRequestType_LoadTrigger)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_UnloadTrigger)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_PlayFile)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_StopFile)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_ExecuteTrigger)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_ExecuteTriggerEx)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_StopTrigger)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_StopAllTriggers)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetTransformation)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetParameter)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetSwitchState)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetCurrentEnvironments)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetEnvironment)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_ResetEnvironments)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_RegisterObject)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_ReleaseObject)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_ProcessPhysicsRay)
				AO_REQUEST_BLOCK(eAudioObjectRequestType_SetName)
			default:
				{
					g_audioLogger.Log(eAudioLogType_Error, "Unknown audio object request type (%u)", pBase->type);
					CRY_ASSERT(false);

					break;
				}
			}
			break;
		}
	case eAudioRequestType_AudioListenerRequest:
		{
			SAudioListenerRequestDataBase const* const pBase = static_cast<SAudioListenerRequestDataBase const* const>(pRequestData);

			switch (pBase->type)
			{
				AL_REQUEST_BLOCK(eAudioListenerRequestType_SetTransformation)
				AL_REQUEST_BLOCK(eAudioListenerRequestType_ReleaseListener)
			default:
				{
					g_audioLogger.Log(eAudioLogType_Error, "Unknown audio listener request type (%u)", pBase->type);
					CRY_ASSERT(false);

					break;
				}
			}

			break;
		}
	case eAudioRequestType_AudioCallbackManagerRequest:
		{
			SAudioCallbackManagerRequestDataBase const* const pBase = static_cast<SAudioCallbackManagerRequestDataBase const* const>(pRequestData);

			switch (pBase->type)
			{
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportStartedEvent)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportFinishedEvent)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportStartedFile)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportStoppedFile)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportVirtualizedEvent)
				ACM_REQUEST_BLOCK(eAudioCallbackManagerRequestType_ReportPhysicalizedEvent)
			default:
				{
					g_audioLogger.Log(eAudioLogType_Error, "Unknown audio callback manager request type (%u)", pBase->type);
					CRY_ASSERT(false);

					break;
				}
			}

			break;
		}
	default:
		{
			g_audioLogger.Log(eAudioLogType_Error, "Unknown audio request type (%u)", requestType);
			CRY_ASSERT(false);

			break;
		}
	}

	return pResult;
}
