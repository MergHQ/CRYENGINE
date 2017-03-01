// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATL.h"
#include "AudioImpl.h"
#include "AudioCVars.h"
#include "ATLAudioObject.h"
#include "PoolObject_impl.h"
#include <CrySystem/ISystem.h>
#include <CryPhysics/IPhysics.h>
#include <CryRenderer/IRenderer.h>
#include <CryString/CryPath.h>
#include <CryEntitySystem/IEntitySystem.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Null;

///////////////////////////////////////////////////////////////////////////
inline ERequestResult ConvertToRequestResult(ERequestStatus const eAudioRequestStatus)
{
	ERequestResult result;

	switch (eAudioRequestStatus)
	{
	case eRequestStatus_Success:
		{
			result = eRequestResult_Success;
			break;
		}
	case eRequestStatus_Failure:
	case eRequestStatus_FailureInvalidObjectId:
	case eRequestStatus_FailureInvalidControlId:
	case eRequestStatus_FailureInvalidRequest:
	case eRequestStatus_PartialSuccess:
		{
			result = eRequestResult_Failure;
			break;
		}
	default:
		{
			g_audioLogger.Log(eAudioLogType_Error, "Invalid AudioRequestStatus '%u'. Cannot be converted to an AudioRequestResult. ", eAudioRequestStatus);
			CRY_ASSERT(false);
			result = eRequestResult_Failure;
			break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
CAudioTranslationLayer::CAudioTranslationLayer()
	: m_audioObjectMgr(m_audioEventMgr, m_audioStandaloneFileMgr, m_audioListenerMgr)
	, m_fileCacheMgr(m_preloadRequests)
	, m_xmlProcessor(m_triggers, m_parameters, m_switches, m_environments, m_preloadRequests, m_fileCacheMgr, m_internalControls)
{

	if (g_audioCVars.m_audioObjectPoolSize < 1)
	{
		g_audioCVars.m_audioObjectPoolSize = 1;
		g_audioLogger.Log(eAudioLogType_Warning, "Audio Object pool size should be greater than zero. Forcing the cvar \"s_AudioObjectPoolSize\" to 1!");
	}
	CATLAudioObject::CreateAllocator(g_audioCVars.m_audioObjectPoolSize);

	if (g_audioCVars.m_audioEventPoolSize < 1)
	{
		g_audioCVars.m_audioEventPoolSize = 1;
		g_audioLogger.Log(eAudioLogType_Warning, "Audio Event pool size should be greater than zero. Forcing the cvar \"s_AudioEventPoolSize\" to 1!");
	}
	CATLEvent::CreateAllocator(g_audioCVars.m_audioEventPoolSize);

	if (g_audioCVars.m_audioStandaloneFilePoolSize < 1)
	{
		g_audioCVars.m_audioStandaloneFilePoolSize = 1;
		g_audioLogger.Log(eAudioLogType_Warning, "Audio Standalone File pool size should be greater than zero. Forcing the cvar \"s_AudioStandaloneFilePoolSize\" to 1!");
	}
	CATLStandaloneFile::CreateAllocator(g_audioCVars.m_audioStandaloneFilePoolSize);
}

//////////////////////////////////////////////////////////////////////////
CAudioTranslationLayer::~CAudioTranslationLayer()
{
	if (m_pImpl != nullptr)
	{
		// the ATL has not yet been properly shut down
		ShutDown();
	}

	CRY_ASSERT_MESSAGE(!m_pGlobalAudioObject, "Global audio object should have been destroyed in the audio thread, before the ATL is destructed.");
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::Initialize(CSystem* const pAudioSystem)
{
	// Add the callback for the obstruction calculation.
	gEnv->pPhysicalWorld->AddEventClient(
	  EventPhysRWIResult::id,
	  &CPropagationProcessor::OnObstructionTest,
	  1);

	CATLAudioObject::s_pEventManager = &m_audioEventMgr;
	CATLAudioObject::s_pAudioSystem = pAudioSystem;
	CATLAudioObject::s_pStandaloneFileManager = &m_audioStandaloneFileMgr;

	return true;
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::ShutDown()
{
	if (gEnv->pPhysicalWorld != nullptr)
	{
		// remove the callback for the obstruction calculation
		gEnv->pPhysicalWorld->RemoveEventClient(
		  EventPhysRWIResult::id,
		  &CPropagationProcessor::OnObstructionTest,
		  1);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::ProcessRequest(CAudioRequest& request)
{
	ERequestStatus result = eRequestStatus_None;

	if (request.GetData())
	{
		switch (request.GetData()->type)
		{
		case eAudioRequestType_AudioObjectRequest:
			{
				result = ProcessAudioObjectRequest(request);

				break;
			}
		case eAudioRequestType_AudioListenerRequest:
			{
				result = ProcessAudioListenerRequest(request.GetData());
				break;
			}
		case eAudioRequestType_AudioCallbackManagerRequest:
			{
				result = ProcessAudioCallbackManagerRequest(request);

				break;
			}
		case eAudioRequestType_AudioManagerRequest:
			{
				result = ProcessAudioManagerRequest(request);

				break;
			}
		default:
			{
				g_audioLogger.Log(eAudioLogType_Error, "Unknown audio request type: %d", static_cast<int>(request.GetData()->type));
				CRY_ASSERT(false);

				break;
			}
		}
	}

	request.status = result;
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::Update(float const deltaTime)
{
	if (m_pImpl != nullptr)
	{
		if (m_lastMainThreadFrameId != gEnv->nMainFrameID)
		{
			g_lastMainThreadFrameStartTime = gEnv->pTimer->GetFrameStartTime();
			m_lastMainThreadFrameId = gEnv->nMainFrameID;

			if (g_audioCVars.m_tickWithMainThread > 0)
			{
				m_audioListenerMgr.Update(deltaTime);
				m_audioEventMgr.Update(deltaTime);
				m_audioObjectMgr.Update(deltaTime, m_audioListenerMgr.GetActiveListenerAttributes());
				m_pGlobalAudioObject->GetImplDataPtr()->Update();
				m_fileCacheMgr.Update();

				m_pImpl->Update(deltaTime);
			}
		}

		if (g_audioCVars.m_tickWithMainThread == 0)
		{
			m_audioListenerMgr.Update(deltaTime);
			m_audioEventMgr.Update(deltaTime);
			m_audioObjectMgr.Update(deltaTime, m_audioListenerMgr.GetActiveListenerAttributes());
			m_pGlobalAudioObject->GetImplDataPtr()->Update();
			m_fileCacheMgr.Update();

			m_pImpl->Update(deltaTime);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioTriggerId(char const* const szAudioTriggerName, ControlId& audioTriggerId) const
{
	ControlId const tempAudioTriggerId = static_cast<ControlId const>(AudioStringToId(szAudioTriggerName));

	if (stl::find_in_map(m_triggers, tempAudioTriggerId, nullptr) != nullptr)
	{
		audioTriggerId = tempAudioTriggerId;
	}
	else
	{
		audioTriggerId = InvalidControlId;
	}

	return (audioTriggerId != InvalidControlId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioParameterId(char const* const szAudioParameterName, ControlId& audioParameterId) const
{
	ControlId const parameterId = static_cast<ControlId const>(AudioStringToId(szAudioParameterName));

	if (stl::find_in_map(m_parameters, parameterId, nullptr) != nullptr)
	{
		audioParameterId = parameterId;
	}
	else
	{
		audioParameterId = InvalidControlId;
	}

	return (audioParameterId != InvalidControlId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioSwitchId(char const* const szAudioSwitchName, ControlId& audioSwitchId) const
{
	ControlId const switchId = static_cast<ControlId const>(AudioStringToId(szAudioSwitchName));

	if (stl::find_in_map(m_switches, switchId, nullptr) != nullptr)
	{
		audioSwitchId = switchId;
	}
	else
	{
		audioSwitchId = InvalidControlId;
	}

	return (audioSwitchId != InvalidControlId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioSwitchStateId(
  ControlId const switchId,
  char const* const szAudioSwitchStateName,
  SwitchStateId& audioSwitchStateId) const
{
	audioSwitchStateId = InvalidControlId;

	CATLSwitch const* const pSwitch = stl::find_in_map(m_switches, switchId, nullptr);

	if (pSwitch != nullptr)
	{
		SwitchStateId const nStateID = static_cast<SwitchStateId const>(AudioStringToId(szAudioSwitchStateName));

		CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, nStateID, nullptr);

		if (pState != nullptr)
		{
			audioSwitchStateId = nStateID;
		}
	}

	return (audioSwitchStateId != InvalidControlId);
}

//////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioPreloadRequestId(char const* const szAudioPreloadRequestName, PreloadRequestId& audioPreloadRequestId) const
{
	PreloadRequestId const nAudioPreloadRequestID = static_cast<PreloadRequestId const>(AudioStringToId(szAudioPreloadRequestName));

	if (stl::find_in_map(m_preloadRequests, nAudioPreloadRequestID, nullptr) != nullptr)
	{
		audioPreloadRequestId = nAudioPreloadRequestID;
	}
	else
	{
		audioPreloadRequestId = InvalidPreloadRequestId;
	}

	return (audioPreloadRequestId != InvalidPreloadRequestId);
}

///////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::GetAudioEnvironmentId(char const* const szAudioEnvironmentName, EnvironmentId& audioEnvironmentId) const
{
	EnvironmentId const environmentId = static_cast<ControlId const>(AudioStringToId(szAudioEnvironmentName));

	if (stl::find_in_map(m_environments, environmentId, nullptr) != nullptr)
	{
		audioEnvironmentId = environmentId;
	}
	else
	{
		audioEnvironmentId = InvalidControlId;
	}

	return (audioEnvironmentId != InvalidControlId);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ParseControlsData(char const* const szFolderPath, EDataScope const dataScope)
{
	m_xmlProcessor.ParseControlsData(szFolderPath, dataScope);

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ParsePreloadsData(char const* const szFolderPath, EDataScope const dataScope)
{
	m_xmlProcessor.ParsePreloadsData(szFolderPath, dataScope);

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ClearControlsData(EDataScope const dataScope)
{
	m_xmlProcessor.ClearControlsData(dataScope);
	if (dataScope == eDataScope_All || dataScope == eDataScope_Global)
	{
		InitInternalControls();
	}

	return eRequestStatus_Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ClearPreloadsData(EDataScope const dataScope)
{
	m_xmlProcessor.ClearPreloadsData(dataScope);

	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::NotifyListener(CAudioRequest const& request)
{
	EnumFlagsType audioSystemEvent = InvalidEnumFlagType;
	CATLStandaloneFile* pStandaloneFile = nullptr;
	ControlId audioControlID = InvalidControlId;
	CATLEvent* pAudioEvent = nullptr;
	switch (request.GetData()->type)
	{
	case eAudioRequestType_AudioManagerRequest:
		{
			SAudioManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioManagerRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case eAudioManagerRequestType_SetAudioImpl:
				audioSystemEvent = eSystemEvent_ImplSet;
				break;
			}

			break;
		}
	case eAudioRequestType_AudioCallbackManagerRequest:
		{
			SAudioCallbackManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioCallbackManagerRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance:
				{
					SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance> const* const>(pRequestDataBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = eSystemEvent_TriggerFinished;

					break;
				}
			case eAudioCallbackManagerRequestType_ReportStartedEvent:
				{
					SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const>(pRequestDataBase);
					pAudioEvent = &pRequestData->audioEvent;

					break;
				}
			case eAudioCallbackManagerRequestType_ReportStartedFile:
				{
					SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const>(pRequestDataBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = eSystemEvent_FileStarted;

					break;
				}
			case eAudioCallbackManagerRequestType_ReportStoppedFile:
				{
					SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const pRequestData = static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const>(pRequestDataBase);
					pStandaloneFile = &pRequestData->audioStandaloneFile;
					audioSystemEvent = eSystemEvent_FileStopped;

					break;
				}
			}

			break;
		}
	case eAudioRequestType_AudioObjectRequest:
		{
			SAudioObjectRequestDataBase const* const pRequestDataBase = static_cast<SAudioObjectRequestDataBase const* const>(request.GetData());

			switch (pRequestDataBase->type)
			{
			case eAudioObjectRequestType_ExecuteTrigger:
				{
					SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const pRequestData = static_cast<SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const>(pRequestDataBase);
					audioControlID = pRequestData->audioTriggerId;
					audioSystemEvent = eSystemEvent_TriggerExecuted;

					break;
				}
			case eAudioObjectRequestType_PlayFile:
				audioSystemEvent = eSystemEvent_FilePlay;
				break;
			}

			break;
		}
	case eAudioRequestType_AudioListenerRequest:
		{
			// Nothing to do currently for this type of request.

			break;
		}
	default:
		{
			CryFatalError("Unknown request type during CAudioTranslationLayer::NotifyListener!");

			break;
		}
	}

	SRequestInfo const requestInfo(
	  ConvertToRequestResult(request.status),
	  request.pOwner,
	  request.pUserData,
	  request.pUserDataOwner,
	  audioSystemEvent,
	  audioControlID,
	  request.pObject,
	  pStandaloneFile,
	  pAudioEvent);

	m_audioEventListenerMgr.NotifyListener(&requestInfo);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ProcessAudioManagerRequest(CAudioRequest const& request)
{
	ERequestStatus result = eRequestStatus_Failure;
	SAudioManagerRequestDataBase const* const pRequestDataBase = static_cast<SAudioManagerRequestDataBase const*>(request.GetData());

	switch (pRequestDataBase->type)
	{
	case eAudioManagerRequestType_ConstructAudioListener:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioListener> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ConstructAudioListener> const*>(request.GetData());
			*pRequestData->ppListener = m_audioListenerMgr.CreateListener();

			break;
		}
	case eAudioManagerRequestType_AddRequestListener:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_AddRequestListener> const*>(request.GetData());
			result = m_audioEventListenerMgr.AddRequestListener(pRequestData);

			break;
		}
	case eAudioManagerRequestType_RemoveRequestListener:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_RemoveRequestListener> const*>(request.GetData());
			result = m_audioEventListenerMgr.RemoveRequestListener(pRequestData->func, pRequestData->pObjectToListenTo);

			break;
		}
	case eAudioManagerRequestType_SetAudioImpl:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_SetAudioImpl> const*>(request.GetData());
			result = SetImpl(pRequestData->pImpl);

			break;
		}
	case eAudioManagerRequestType_RefreshAudioSystem:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_RefreshAudioSystem> const*>(request.GetData());
			result = RefreshAudioSystem(pRequestData->levelName);

			break;
		}
	case eAudioManagerRequestType_LoseFocus:
		{
			if (
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			  (g_audioCVars.m_ignoreWindowFocus == 0) &&
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
			  (m_flags & eAudioInternalStates_IsMuted) == 0)
			{
				CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, LoseFocusTriggerId, nullptr);

				if (pTrigger != nullptr)
				{
					m_pGlobalAudioObject->HandleExecuteTrigger(pTrigger);
				}
				else
				{
					g_audioLogger.Log(eAudioLogType_Warning, "Could not find definition of lose focus trigger");
				}

				result = m_pImpl->OnLoseFocus();
			}

			break;
		}
	case eAudioManagerRequestType_GetFocus:
		{
			if (
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			  (g_audioCVars.m_ignoreWindowFocus == 0) &&
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
			  (m_flags & eAudioInternalStates_IsMuted) == 0)
			{
				result = m_pImpl->OnGetFocus();
				CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, GetFocusTriggerId, nullptr);

				if (pTrigger != nullptr)
				{
					m_pGlobalAudioObject->HandleExecuteTrigger(pTrigger);
				}
				else
				{
					g_audioLogger.Log(eAudioLogType_Warning, "Could not find definition of get focus trigger");
				}
			}

			break;
		}
	case eAudioManagerRequestType_MuteAll:
		{
			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, MuteAllTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				m_pGlobalAudioObject->HandleExecuteTrigger(pTrigger);
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "Could not find definition of mute all trigger");
			}

			result = m_pImpl->MuteAll();

			if (result == eRequestStatus_Success)
			{
				m_flags |= eAudioInternalStates_IsMuted;
			}
			else
			{
				m_flags &= ~eAudioInternalStates_IsMuted;
			}

			break;
		}
	case eAudioManagerRequestType_UnmuteAll:
		{
			result = m_pImpl->UnmuteAll();

			if (result != eRequestStatus_Success)
			{
				m_flags |= eAudioInternalStates_IsMuted;
			}
			else
			{
				m_flags &= ~eAudioInternalStates_IsMuted;
			}

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, UnmuteAllTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				m_pGlobalAudioObject->HandleExecuteTrigger(pTrigger);
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "Could not find definition of unmute trigger");
			}

			break;
		}
	case eAudioManagerRequestType_StopAllSounds:
		{
			result = m_pImpl->StopAllSounds();

			break;
		}
	case eAudioManagerRequestType_ParseControlsData:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ParseControlsData> const*>(request.GetData());
			result = ParseControlsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			break;
		}
	case eAudioManagerRequestType_ParsePreloadsData:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ParsePreloadsData> const*>(request.GetData());
			result = ParsePreloadsData(pRequestData->folderPath.c_str(), pRequestData->dataScope);

			break;
		}
	case eAudioManagerRequestType_ClearControlsData:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ClearControlsData> const*>(request.GetData());
			result = ClearControlsData(pRequestData->dataScope);

			break;
		}
	case eAudioManagerRequestType_ClearPreloadsData:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ClearPreloadsData> const*>(request.GetData());
			result = ClearPreloadsData(pRequestData->dataScope);

			break;
		}
	case eAudioManagerRequestType_PreloadSingleRequest:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_PreloadSingleRequest> const*>(request.GetData());
			result = m_fileCacheMgr.TryLoadRequest(pRequestData->audioPreloadRequestId, ((request.flags & eRequestFlags_ExecuteBlocking) != 0), pRequestData->bAutoLoadOnly);

			break;
		}
	case eAudioManagerRequestType_UnloadSingleRequest:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_UnloadSingleRequest> const*>(request.GetData());
			result = m_fileCacheMgr.TryUnloadRequest(pRequestData->audioPreloadRequestId);

			break;
		}
	case eAudioManagerRequestType_UnloadAFCMDataByScope:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_UnloadAFCMDataByScope> const*>(request.GetData());
			result = m_fileCacheMgr.UnloadDataByScope(pRequestData->dataScope);

			break;
		}
	case eAudioManagerRequestType_ReleaseAudioImpl:
		{
			ReleaseImpl();
			result = eRequestStatus_Success;

			break;
		}
	case eAudioManagerRequestType_ChangeLanguage:
		{
			SetImplLanguage();

			m_fileCacheMgr.UpdateLocalizedFileCacheEntries();
			result = eRequestStatus_Success;

			break;
		}
	case eAudioManagerRequestType_RetriggerAudioControls:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			RetriggerAudioControls();
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
			result = eRequestStatus_Success;

			break;
		}
	case eAudioManagerRequestType_ReleasePendingRays:
		{
			m_audioObjectMgr.ReleasePendingRays();
			result = eRequestStatus_Success;

			break;
		}
	case eAudioManagerRequestType_ReloadControlsData:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> const* const pRequestData = static_cast<SAudioManagerRequestData<eAudioManagerRequestType_ReloadControlsData> const*>(request.GetData());
			for (auto pAudioObject : m_audioObjectMgr.GetAudioObjects())
			{
				for (auto pEvent : pAudioObject->GetActiveEvents())
				{
					if (pEvent != nullptr)
					{
						result = pEvent->Reset();
					}
				}
			}

			if (ClearControlsData(eDataScope_All) == eRequestStatus_Success)
			{
				if (ParseControlsData(pRequestData->folderPath, eDataScope_Global) == eRequestStatus_Success)
				{
					if (strcmp(pRequestData->levelName, "") != 0)
					{
						if (ParseControlsData(pRequestData->folderPath + pRequestData->levelName, eDataScope_LevelSpecific) == eRequestStatus_Success)
						{
							RetriggerAudioControls();
							result = eRequestStatus_Success;
						}
					}
					else
					{
						RetriggerAudioControls();
						result = eRequestStatus_Success;
					}
				}
			}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
			break;
		}
	case eAudioManagerRequestType_DrawDebugInfo:
		{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			DrawAudioSystemDebugInfo();
			result = eRequestStatus_Success;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	case eAudioManagerRequestType_GetAudioFileData:
		{
			SAudioManagerRequestData<eAudioManagerRequestType_GetAudioFileData> const* const pRequestData =
			  static_cast<SAudioManagerRequestData<eAudioManagerRequestType_GetAudioFileData> const* const>(request.GetData());
			m_pImpl->GetAudioFileData(pRequestData->szFilename, pRequestData->audioFileData);
			break;
		}
	case eAudioManagerRequestType_None:
		{
			result = eRequestStatus_Success;

			break;
		}
	default:
		{
			g_audioLogger.Log(eAudioLogType_Warning, "ATL received an unknown AudioManager request: %u", pRequestDataBase->type);
			result = eRequestStatus_FailureInvalidRequest;

			break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ProcessAudioCallbackManagerRequest(CAudioRequest& request)
{
	ERequestStatus result = eRequestStatus_Failure;
	SAudioCallbackManagerRequestDataBase const* const pRequestDataBase =
	  static_cast<SAudioCallbackManagerRequestDataBase const* const>(request.GetData());

	switch (pRequestDataBase->type)
	{
	case eAudioCallbackManagerRequestType_ReportStartedEvent:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedEvent> const* const>(request.GetData());
			CATLEvent& audioEvent = pRequestData->audioEvent;

			audioEvent.m_audioEventState = eAudioEventState_PlayingDelayed;

			if (audioEvent.m_pAudioObject != m_pGlobalAudioObject)
			{
				m_audioObjectMgr.ReportStartedEvent(&audioEvent);
			}
			else
			{
				m_pGlobalAudioObject->ReportStartedEvent(&audioEvent);
			}

			result = eRequestStatus_Success;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportFinishedEvent:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportFinishedEvent> const* const>(request.GetData());
			CATLEvent& audioEvent = pRequestData->audioEvent;

			if (audioEvent.m_pAudioObject != m_pGlobalAudioObject)
			{
				m_audioObjectMgr.ReportFinishedEvent(&audioEvent, pRequestData->bSuccess);
			}
			else
			{
				m_pGlobalAudioObject->ReportFinishedEvent(&audioEvent, pRequestData->bSuccess);
			}

			m_audioEventMgr.ReleaseAudioEvent(&audioEvent);

			result = eRequestStatus_Success;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportVirtualizedEvent:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportVirtualizedEvent> const* const>(request.GetData());

			pRequestData->audioEvent.m_audioEventState = eAudioEventState_Virtual;

			result = eRequestStatus_Success;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportPhysicalizedEvent:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportPhysicalizedEvent> const* const>(request.GetData());

			pRequestData->audioEvent.m_audioEventState = eAudioEventState_Playing;

			result = eRequestStatus_Success;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportStartedFile:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStartedFile> const* const>(request.GetData());

			CATLStandaloneFile& audioStandaloneFile = pRequestData->audioStandaloneFile;

			m_audioObjectMgr.GetStartedStandaloneFileRequestData(&audioStandaloneFile, request);
			audioStandaloneFile.m_state = (pRequestData->bSuccess) ? eAudioStandaloneFileState_Playing : eAudioStandaloneFileState_None;

			result = (pRequestData->bSuccess) ? eRequestStatus_Success : eRequestStatus_Failure;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportStoppedFile:
		{
			SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const pRequestData =
			  static_cast<SAudioCallbackManagerRequestData<eAudioCallbackManagerRequestType_ReportStoppedFile> const* const>(request.GetData());

			CATLStandaloneFile& audioStandaloneFile = pRequestData->audioStandaloneFile;

			m_audioObjectMgr.GetStartedStandaloneFileRequestData(&audioStandaloneFile, request);

			if (audioStandaloneFile.m_pAudioObject != m_pGlobalAudioObject)
			{
				m_audioObjectMgr.ReportFinishedStandaloneFile(&audioStandaloneFile);
			}
			else
			{
				m_pGlobalAudioObject->ReportFinishedStandaloneFile(&audioStandaloneFile);
			}

			m_audioStandaloneFileMgr.ReleaseStandaloneFile(&audioStandaloneFile);

			result = eRequestStatus_Success;

			break;
		}
	case eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance:
	case eAudioCallbackManagerRequestType_None:
		{
			result = eRequestStatus_Success;

			break;
		}
	default:
		{
			result = eRequestStatus_FailureInvalidRequest;
			g_audioLogger.Log(eAudioLogType_Warning, "ATL received an unknown AudioCallbackManager request: %u", pRequestDataBase->type);

			break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ProcessAudioObjectRequest(CAudioRequest const& request)
{
	ERequestStatus result = eRequestStatus_Failure;
	CATLAudioObject* pObject = m_pGlobalAudioObject;

	if (request.pObject != nullptr)
	{
		pObject = request.pObject;
	}

	SAudioObjectRequestDataBase const* const pBaseRequestData =
	  static_cast<SAudioObjectRequestDataBase const* const>(request.GetData());

	switch (pBaseRequestData->type)
	{
	case eAudioObjectRequestType_LoadTrigger:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_LoadTrigger> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_LoadTrigger> const* const>(request.GetData());

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->LoadTriggerAsync(pTrigger, true);
			}
			else
			{
				result = eRequestStatus_FailureInvalidControlId;
			}

			break;
		}
	case eAudioObjectRequestType_UnloadTrigger:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_UnloadTrigger> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_UnloadTrigger> const* const>(request.GetData());

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->LoadTriggerAsync(pTrigger, false);
			}
			else
			{
				result = eRequestStatus_FailureInvalidControlId;
			}

			break;
		}
	case eAudioObjectRequestType_PlayFile:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> const* const>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				IAudioTrigger const* pTriggerImpl = nullptr;
				if (pRequestData->usedAudioTriggerId != InvalidControlId)
				{
					CATLTrigger const* const pAudioTrigger = stl::find_in_map(m_triggers, pRequestData->usedAudioTriggerId, nullptr);
					if (pAudioTrigger != nullptr && !pAudioTrigger->m_implPtrs.empty())
					{
						pTriggerImpl = pAudioTrigger->m_implPtrs[0]->m_pImplData;
					}
				}

				CATLStandaloneFile* const pFile = m_audioStandaloneFileMgr.ConstructStandaloneFile(pRequestData->file.c_str(), pRequestData->bLocalized, pTriggerImpl);
				result = pObject->HandlePlayFile(pFile, request.pOwner, request.pUserData, request.pUserDataOwner);
			}

			break;
		}
	case eAudioObjectRequestType_StopFile:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_StopFile> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_StopFile> const* const>(request.GetData());

			if (pRequestData != nullptr && !pRequestData->file.empty())
			{
				result = pObject->HandleStopFile(pRequestData->file.c_str());
			}

			break;
		}
	case eAudioObjectRequestType_ExecuteTrigger:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> const* const>(request.GetData());

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->HandleExecuteTrigger(pTrigger, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
			}
			else
			{
				result = eRequestStatus_FailureInvalidControlId;
			}

			break;
		}
	case eAudioObjectRequestType_ExecuteTriggerEx:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTriggerEx> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTriggerEx> const* const>(request.GetData());

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, pRequestData->triggerId, nullptr);

			if (pTrigger != nullptr)
			{
				CATLAudioObject* const pObject = new CATLAudioObject;
				m_audioObjectMgr.RegisterObject(pObject);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pObject->Init(pRequestData->name.c_str(), m_pImpl->ConstructAudioObject(pRequestData->name.c_str()), m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition());
#else
				pObject->Init(nullptr, m_pImpl->ConstructAudioObject(nullptr), m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

				result = pObject->HandleSetTransformation(pRequestData->transformation, 0.0f);

				if (pRequestData->bSetCurrentEnvironments)
				{
					SetCurrentEnvironmentsOnObject(pObject, INVALID_ENTITYID, pRequestData->transformation.GetPosition());
				}

				EnumFlagsType const index = static_cast<EnumFlagsType>(pRequestData->occlusionType);

				if (index < eOcclusionType_Count)
				{
					// TODO: Can we prevent these lookups and access the switch and state directly?
					CATLSwitch const* const pSwitch = stl::find_in_map(m_switches, OcclusionTypeSwitchId, nullptr);

					if (pSwitch != nullptr)
					{
						CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, OcclusionTypeStateIds[index], nullptr);

						if (pState != nullptr)
						{
							result = pObject->HandleSetSwitchState(pSwitch, pState);
						}
					}
				}

				result = pObject->HandleExecuteTrigger(pTrigger, request.pOwner, request.pUserData, request.pUserDataOwner, request.flags);
				pObject->RemoveFlag(eAudioObjectFlags_DoNotRelease);
			}
			else
			{
				result = eRequestStatus_FailureInvalidControlId;
			}

			break;
		}
	case eAudioObjectRequestType_StopTrigger:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> const* const>(request.GetData());

			CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, pRequestData->audioTriggerId, nullptr);

			if (pTrigger != nullptr)
			{
				result = pObject->HandleStopTrigger(pTrigger);
			}
			else
			{
				result = eRequestStatus_FailureInvalidControlId;
			}

			break;
		}
	case eAudioObjectRequestType_StopAllTriggers:
		{
			result = pObject->StopAllTriggers();
			;

			break;
		}
	case eAudioObjectRequestType_SetTransformation:
		{
			if (pObject != m_pGlobalAudioObject)
			{
				float const distanceToListener = (m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition() - pObject->GetTransformation().GetPosition()).GetLength();
				SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> const* const pRequestData =
				  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> const* const>(request.GetData());

				result = pObject->HandleSetTransformation(pRequestData->transformation, distanceToListener);
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "ATL received a request to set a position on a global object");
			}

			break;
		}
	case eAudioObjectRequestType_SetParameter:
		{
			result = eRequestStatus_FailureInvalidControlId;

			SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_SetParameter> const* const>(request.GetData());

			CParameter const* const pParameter = stl::find_in_map(m_parameters, pRequestData->parameterId, nullptr);

			if (pParameter != nullptr)
			{
				result = pObject->HandleSetParameter(pParameter, pRequestData->value);
			}

			break;
		}
	case eAudioObjectRequestType_SetSwitchState:
		{
			result = eRequestStatus_FailureInvalidControlId;
			SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> const* const>(request.GetData());

			CATLSwitch const* const pSwitch = stl::find_in_map(m_switches, pRequestData->audioSwitchId, nullptr);

			if (pSwitch != nullptr)
			{
				CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, pRequestData->audioSwitchStateId, nullptr);

				if (pState != nullptr)
				{
					result = pObject->HandleSetSwitchState(pSwitch, pState);
				}
			}

			break;
		}
	case eAudioObjectRequestType_SetCurrentEnvironments:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_SetCurrentEnvironments> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_SetCurrentEnvironments> const* const>(request.GetData());

			SetCurrentEnvironmentsOnObject(pObject, pRequestData->entityToIgnore, pRequestData->position);

			break;
		}
	case eAudioObjectRequestType_SetEnvironment:
		{
			if (pObject != m_pGlobalAudioObject)
			{
				result = eRequestStatus_FailureInvalidControlId;
				SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironment> const* const pRequestData =
				  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironment> const* const>(request.GetData());

				CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(m_environments, pRequestData->audioEnvironmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					result = pObject->HandleSetEnvironment(pEnvironment, pRequestData->amount);
				}
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "ATL received a request to set an environment on a global object");
			}

			break;
		}
	case eAudioObjectRequestType_ResetEnvironments:
		{
			result = pObject->HandleResetEnvironments(m_environments);
			break;
		}
	case eAudioObjectRequestType_RegisterObject:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_RegisterObject> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_RegisterObject> const*>(request.GetData());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pObject->Init(pRequestData->name.c_str(), m_pImpl->ConstructAudioObject(pRequestData->name.c_str()), m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition());
#else
			pObject->Init(nullptr, m_pImpl->ConstructAudioObject(nullptr), m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition());
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			result = pObject->HandleSetTransformation(pRequestData->transformation, 0.0f);
			CRY_ASSERT(result == eRequestStatus_Success);

			if (pRequestData->bSetCurrentEnvironments)
			{
				SetCurrentEnvironmentsOnObject(pObject, INVALID_ENTITYID, pRequestData->transformation.GetPosition());
			}

			EnumFlagsType const index = static_cast<EnumFlagsType>(pRequestData->occlusionType);

			if (index < eOcclusionType_Count)
			{
				// TODO: Can we prevent these lookups and access the switch and state directly?
				CATLSwitch const* const pSwitch = stl::find_in_map(m_switches, OcclusionTypeSwitchId, nullptr);

				if (pSwitch != nullptr)
				{
					CATLSwitchState const* const pState = stl::find_in_map(pSwitch->audioSwitchStates, OcclusionTypeStateIds[index], nullptr);

					if (pState != nullptr)
					{
						result = pObject->HandleSetSwitchState(pSwitch, pState);
						CRY_ASSERT(result == eRequestStatus_Success);
					}
				}
			}

			m_audioObjectMgr.RegisterObject(pObject);
			result = eRequestStatus_Success;

			break;
		}
	case eAudioObjectRequestType_ReleaseObject:
		{
			if (pObject != m_pGlobalAudioObject)
			{
				pObject->RemoveFlag(eAudioObjectFlags_DoNotRelease);
				result = eRequestStatus_Success;
			}
			else
			{
				g_audioLogger.Log(eAudioLogType_Warning, "ATL received a request to release the GlobalAudioObject");
			}

			break;
		}
	case eAudioObjectRequestType_ProcessPhysicsRay:
		{
			SAudioObjectRequestData<eAudioObjectRequestType_ProcessPhysicsRay> const* const pRequestData =
			  static_cast<SAudioObjectRequestData<eAudioObjectRequestType_ProcessPhysicsRay> const* const>(request.GetData());

			pObject->ProcessPhysicsRay(pRequestData->pAudioRayInfo);
			result = eRequestStatus_Success;
			break;
		}
	case eAudioObjectRequestType_None:
		{
			result = eRequestStatus_Success;

			break;
		}
	default:
		{
			result = eRequestStatus_FailureInvalidRequest;
			g_audioLogger.Log(eAudioLogType_Warning, "ATL received an unknown AudioObject request type: %u", pBaseRequestData->type);

			break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::ProcessAudioListenerRequest(SAudioRequestData const* const pPassedRequestData)
{
	ERequestStatus result = eRequestStatus_Failure;
	SAudioListenerRequestDataBase const* const pBaseRequestData =
	  static_cast<SAudioListenerRequestDataBase const* const>(pPassedRequestData);

	switch (pBaseRequestData->type)
	{
	case eAudioListenerRequestType_SetTransformation:
		{
			SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> const* const pRequestData =
			  static_cast<SAudioListenerRequestData<eAudioListenerRequestType_SetTransformation> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				result = pRequestData->pListener->HandleSetTransformation(pRequestData->transformation);
			}
		}
		break;
	case eAudioListenerRequestType_ReleaseListener:
		{
			SAudioListenerRequestData<eAudioListenerRequestType_ReleaseListener> const* const pRequestData =
			  static_cast<SAudioListenerRequestData<eAudioListenerRequestType_ReleaseListener> const* const>(pPassedRequestData);

			CRY_ASSERT(pRequestData->pListener != nullptr);

			if (pRequestData->pListener != nullptr)
			{
				m_audioListenerMgr.ReleaseListener(pRequestData->pListener);
				result = eRequestStatus_Success;
			}

			break;
		}
	case eAudioListenerRequestType_None:
		result = eRequestStatus_Success;
		break;
	default:
		result = eRequestStatus_FailureInvalidRequest;
		g_audioLogger.Log(eAudioLogType_Warning, "ATL received an unknown AudioListener request type: %u", pPassedRequestData->type);
		break;
	}

	return result;
}
///////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::SetImpl(IAudioImpl* const pImpl)
{
	ERequestStatus result = eRequestStatus_Failure;

	if (m_pImpl != nullptr && pImpl != m_pImpl)
	{
		ReleaseImpl();
	}

	m_pImpl = pImpl;

	if (m_pImpl == nullptr)
	{
		g_audioLogger.Log(eAudioLogType_Warning, "nullptr passed to SetImpl, will run with the null implementation");

		CAudioImpl* pNullImpl = new CAudioImpl();
		CRY_ASSERT(pNullImpl != nullptr);
		m_pImpl = pNullImpl;
	}

	result = m_pImpl->Init(g_audioCVars.m_audioObjectPoolSize, g_audioCVars.m_audioEventPoolSize);

	if (result != eRequestStatus_Success)
	{
		// The impl failed to initialize, allow it to shut down and release then fall back to the null impl.
		g_audioLogger.Log(eAudioLogType_Error, "Failed to set the AudioImpl %s. Will run with the null implementation.", m_pImpl->GetImplementationNameString());

		// There's no need to call Shutdown when the initialization failed as
		// we expect the implementation to clean-up itself if it couldn't be initialized

		result = m_pImpl->Release(); // Release the engine specific data.
		CRY_ASSERT(result == eRequestStatus_Success);

		CAudioImpl* pNullImpl = new CAudioImpl();
		CRY_ASSERT(pNullImpl != nullptr);
		m_pImpl = pNullImpl;
	}

	if (m_pGlobalAudioObject == nullptr)
	{
		m_pGlobalAudioObject = new CATLAudioObject;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_pGlobalAudioObject->m_name = "Global Audio Object";
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	CRY_ASSERT(m_pGlobalAudioObject->GetImplDataPtr() == nullptr);
	m_pGlobalAudioObject->SetImplDataPtr(m_pImpl->ConstructGlobalAudioObject());

	m_audioObjectMgr.Init(m_pImpl);
	m_audioEventMgr.Init(m_pImpl);
	m_audioStandaloneFileMgr.Init(m_pImpl);
	m_audioListenerMgr.Init(m_pImpl);
	m_xmlProcessor.Init(m_pImpl);
	m_fileCacheMgr.Init(m_pImpl);

	CATLControlImpl::SetImpl(m_pImpl);

	SetImplLanguage();
	InitInternalControls();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_implNameString = m_pImpl->GetImplementationNameString();
#endif //INCLUDE_AUDIO_PRODUCTION_CODE

	return result;
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::ReleaseImpl()
{
	// During audio middleware shutdown we do not allow for any new requests originating from the "dying" audio middleware!
	m_flags |= eAudioInternalStates_AudioMiddlewareShuttingDown;

	m_pImpl->OnBeforeShutDown();

	m_audioStandaloneFileMgr.Release();
	m_audioEventMgr.Release();
	m_audioObjectMgr.Release();
	m_audioListenerMgr.Release();

	m_pImpl->DestructAudioObject(m_pGlobalAudioObject->GetImplDataPtr());
	m_pGlobalAudioObject->SetImplDataPtr(nullptr);

	delete m_pGlobalAudioObject;
	m_pGlobalAudioObject = nullptr;

	m_xmlProcessor.ClearControlsData(eDataScope_All);
	m_xmlProcessor.ClearPreloadsData(eDataScope_All);

	m_fileCacheMgr.Release();

	ERequestStatus result = m_pImpl->ShutDown(); // Shut down the audio middleware.
	CRY_ASSERT(result == eRequestStatus_Success);

	result = m_pImpl->Release(); // Release the engine specific data.
	CRY_ASSERT(result == eRequestStatus_Success);

	m_pImpl = nullptr;
	m_flags &= ~eAudioInternalStates_AudioMiddlewareShuttingDown;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioTranslationLayer::RefreshAudioSystem(char const* const szLevelName)
{
	g_audioLogger.Log(eAudioLogType_Warning, "Beginning to refresh the AudioSystem!");

	ERequestStatus result = m_pImpl->StopAllSounds();
	CRY_ASSERT(result == eRequestStatus_Success);

	result = m_fileCacheMgr.UnloadDataByScope(eDataScope_LevelSpecific);
	CRY_ASSERT(result == eRequestStatus_Success);

	result = m_fileCacheMgr.UnloadDataByScope(eDataScope_Global);
	CRY_ASSERT(result == eRequestStatus_Success);

	result = ClearControlsData(eDataScope_All);
	CRY_ASSERT(result == eRequestStatus_Success);

	result = ClearPreloadsData(eDataScope_All);
	CRY_ASSERT(result == eRequestStatus_Success);

	m_pImpl->OnAudioSystemRefresh();

	SetImplLanguage();

	CryFixedStringT<MaxFilePathLength> configPath(gEnv->pAudioSystem->GetConfigPath());
	result = ParseControlsData(configPath.c_str(), eDataScope_Global);
	CRY_ASSERT(result == eRequestStatus_Success);
	result = ParsePreloadsData(configPath.c_str(), eDataScope_Global);
	CRY_ASSERT(result == eRequestStatus_Success);

	// The global preload might not exist if no preloads have been created, for that reason we don't check the result of this call
	result = m_fileCacheMgr.TryLoadRequest(GlobalPreloadRequestId, true, true);

	if (szLevelName != nullptr && szLevelName[0] != '\0')
	{
		configPath += "levels" CRY_NATIVE_PATH_SEPSTR;
		configPath += szLevelName;
		result = ParseControlsData(configPath.c_str(), eDataScope_LevelSpecific);
		CRY_ASSERT(result == eRequestStatus_Success);

		result = ParsePreloadsData(configPath.c_str(), eDataScope_LevelSpecific);
		CRY_ASSERT(result == eRequestStatus_Success);

		PreloadRequestId audioPreloadRequestId = InvalidPreloadRequestId;

		if (GetAudioPreloadRequestId(szLevelName, audioPreloadRequestId))
		{
			result = m_fileCacheMgr.TryLoadRequest(audioPreloadRequestId, true, true);
			CRY_ASSERT(result == eRequestStatus_Success);
		}
	}

	g_audioLogger.Log(eAudioLogType_Warning, "Done refreshing the AudioSystem!");

	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::SetImplLanguage()
{
	if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		m_pImpl->SetLanguage(pCVar->GetString());
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAudioTranslationLayer::OnInputEvent(SInputEvent const& event)
{
	if (event.state == eIS_Changed && event.deviceType == eIDT_Gamepad)
	{
		if (event.keyId == eKI_SYS_ConnectDevice)
		{
			m_pImpl->GamepadConnected(event.deviceUniqueID);
		}
		else if (event.keyId == eKI_SYS_DisconnectDevice)
		{
			m_pImpl->GamepadDisconnected(event.deviceUniqueID);
		}
	}

	// Do not consume event
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::InitInternalControls()
{
	m_internalControls.m_switchStates.clear();
	m_internalControls.m_parameters.clear();

	// Occlusion
	COcclusionObstructionState* pOcclusionIgnore = new COcclusionObstructionState(IgnoreStateId, m_audioListenerMgr, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(OcclusionCalcSwitchId, IgnoreStateId)]
	  = pOcclusionIgnore;

	COcclusionObstructionState* pOcclusionAdaptive = new COcclusionObstructionState(AdaptiveStateId, m_audioListenerMgr, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(OcclusionCalcSwitchId, AdaptiveStateId)]
	  = pOcclusionAdaptive;

	COcclusionObstructionState* pOcclusionLow = new COcclusionObstructionState(LowStateId, m_audioListenerMgr, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(OcclusionCalcSwitchId, LowStateId)]
	  = pOcclusionLow;

	COcclusionObstructionState* pOcclusionMedium = new COcclusionObstructionState(MediumStateId, m_audioListenerMgr, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(OcclusionCalcSwitchId, MediumStateId)]
	  = pOcclusionMedium;

	COcclusionObstructionState* pOcclusionHigh = new COcclusionObstructionState(HighStateId, m_audioListenerMgr, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(OcclusionCalcSwitchId, HighStateId)]
	  = pOcclusionHigh;

	// Relative Velocity
	CDopplerTrackingState* pDopplerOn = new CDopplerTrackingState(OnStateId, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(RelativeVelocityTrackingSwitchId, OnStateId)]
	  = pDopplerOn;

	CDopplerTrackingState* pDopplerOff = new CDopplerTrackingState(OffStateId, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(RelativeVelocityTrackingSwitchId, OffStateId)]
	  = pDopplerOff;

	// Absolute Velocity
	CVelocityTrackingState* pVelocityOn = new CVelocityTrackingState(OnStateId, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(AbsoluteVelocityTrackingSwitchId, OnStateId)]
	  = pVelocityOn;

	CVelocityTrackingState* pVelocityOff = new CVelocityTrackingState(OffStateId, *m_pGlobalAudioObject);
	m_internalControls.m_switchStates[std::make_pair(AbsoluteVelocityTrackingSwitchId, OffStateId)]
	  = pVelocityOff;
}

//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::SetCurrentEnvironmentsOnObject(CATLAudioObject* const pObject, EntityId const entityToIgnore, Vec3 const& position)
{
	IAreaManager* const pIAreaManager = gEnv->pEntitySystem->GetAreaManager();
	size_t numAreas = 0;
	static size_t const s_maxAreas = 10;
	static SAudioAreaInfo s_areaInfos[s_maxAreas];

	// TODO: Clarify whether QueryAudioAreas is thread safe or not!
	if (pIAreaManager->QueryAudioAreas(position, s_areaInfos, s_maxAreas, numAreas))
	{
		for (size_t i = 0; i < numAreas; ++i)
		{
			SAudioAreaInfo const& areaInfo = s_areaInfos[i];

			if (entityToIgnore == INVALID_ENTITYID || entityToIgnore != areaInfo.envProvidingEntityId)
			{
				CATLAudioEnvironment const* const pEnvironment = stl::find_in_map(m_environments, areaInfo.audioEnvironmentId, nullptr);

				if (pEnvironment != nullptr)
				{
					pObject->HandleSetEnvironment(pEnvironment, areaInfo.amount);
				}
			}
		}
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::DrawAudioSystemDebugInfo()
{
	CRY_PROFILE_FUNCTION(PROFILE_AUDIO);

	if (IRenderAuxGeom* const pAuxGeom = g_audioCVars.m_drawAudioDebug > 0 && gEnv->pRenderer != nullptr ? gEnv->pRenderer->GetIRenderAuxGeom() : nullptr)
	{
		DrawAudioObjectDebugInfo(*pAuxGeom); // needs to be called first so that the rest of the labels are printed
		// on top (Draw2dLabel doesn't provide a way set which labels are printed on top)

		float posX = 0.0f;
		float posY = 4.0f;
		float const lineHeight = 17.0f;
		float const indentation = 20.0f;

		static float const s_colorWhite[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
		static float const s_colorRed[4] = { 1.0f, 0.0f, 0.0f, 0.7f };
		static float const s_colorGreen[4] = { 0.0f, 1.0f, 0.0f, 0.7f };
		static float const s_colorBlue[4] = { 0.4f, 0.4f, 1.0f, 1.0f };

		pAuxGeom->Draw2dLabel(posX, posY, 1.6f, s_colorBlue, false, "AudioTranslationLayer with %s", m_implNameString.c_str());
		posX += indentation;
		posY += lineHeight;

		CryModuleMemoryInfo memInfo;
		ZeroStruct(memInfo);
		CryGetMemoryInfoForModule(&memInfo);

		pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false,
		                      "[ATL] Total Memory Used: %uKiB",
		                      static_cast<uint32>((memInfo.allocated - memInfo.freed) / 1024));

		posX += indentation;
		{
			CPoolObject<CATLAudioObject, stl::PSyncMultiThread>::Allocator& allocator = CATLAudioObject::GetAllocator();
			posY += lineHeight;
			auto mem = allocator.GetTotalMemory();
			auto pool = allocator.GetCounts();
			pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false, "[%s] InUse: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", "Objects", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
		}

		{
			CPoolObject<CATLEvent, stl::PSyncNone>::Allocator& allocator = CATLEvent::GetAllocator();
			posY += lineHeight;
			auto mem = allocator.GetTotalMemory();
			auto pool = allocator.GetCounts();
			pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false, "[%s] InUse: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", "Events", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
		}

		{
			CPoolObject<CATLStandaloneFile, stl::PSyncNone>::Allocator& allocator = CATLStandaloneFile::GetAllocator();
			posY += lineHeight;
			auto mem = allocator.GetTotalMemory();
			auto pool = allocator.GetCounts();
			pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false, "[%s] InUse: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB", "Files", pool.nUsed, pool.nAlloc, mem.nUsed / 1024, mem.nAlloc / 1024);
		}
		posX -= indentation;

		if (m_pImpl != nullptr)
		{
			SAudioImplMemoryInfo memoryInfo;
			m_pImpl->GetMemoryInfo(memoryInfo);

			posY += lineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false, "[Impl] Total Memory Used: %uKiB | Secondary Memory: %.2f / %.2f MiB NumAllocs: %d",
			                      static_cast<uint32>(memoryInfo.totalMemory / 1024),
			                      (memoryInfo.secondaryPoolUsedSize / 1024) / 1024.0f,
			                      (memoryInfo.secondaryPoolSize / 1024) / 1024.0f,
			                      static_cast<int>(memoryInfo.secondaryPoolAllocations));

			posX += indentation;
			posY += lineHeight;
			pAuxGeom->Draw2dLabel(posX, posY, 1.35f, s_colorWhite, false, "[Impl Object Pool] InUse: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB",
			                      memoryInfo.poolUsedObjects, memoryInfo.poolConstructedObjects, memoryInfo.poolUsedMemory, memoryInfo.poolAllocatedMemory);
			posX -= indentation;
		}

		posY += lineHeight;
		static float const SMOOTHING_ALPHA = 0.2f;
		static float syncRays = 0;
		static float asyncRays = 0;

		Vec3 const& listenerPosition = m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition();
		Vec3 const& listenerDirection = m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetForward();
		float const listenerVelocity = m_audioListenerMgr.GetActiveListenerAttributes().velocity.GetLength();
		size_t const numObjects = m_audioObjectMgr.GetNumAudioObjects();
		size_t const numActiveObjects = m_audioObjectMgr.GetNumActiveAudioObjects();
		size_t const numEvents = m_audioEventMgr.GetNumConstructed();
		size_t const numListeners = m_audioListenerMgr.GetNumActiveListeners();
		size_t const numEventListeners = m_audioEventListenerMgr.GetNumEventListeners();
		syncRays += (CPropagationProcessor::s_totalSyncPhysRays - syncRays) * SMOOTHING_ALPHA;
		asyncRays += (CPropagationProcessor::s_totalAsyncPhysRays - asyncRays) * SMOOTHING_ALPHA * 0.1f;

		bool const bActive = true;
		float const colorListener[4] =
		{
			bActive ? s_colorGreen[0] : s_colorRed[0],
			bActive ? s_colorGreen[1] : s_colorRed[1],
			bActive ? s_colorGreen[2] : s_colorRed[2],
			1.0f
		};

		float const* colorNumbers = s_colorBlue;

		posY += lineHeight;
		pAuxGeom->Draw2dLabel(posX, posY, 1.35f, colorListener, false, "Listener <%d> PosXYZ: %.2f %.2f %.2f FwdXYZ: %.2f %.2f %.2f Velocity: %.2f m/s", 0, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);

		posY += lineHeight;
		pAuxGeom->Draw2dLabel(posX, posY, 1.35f, colorNumbers, false,
		                      "Objects: %3" PRISIZE_T "/%3" PRISIZE_T " Events: %3" PRISIZE_T " EventListeners %3" PRISIZE_T " Listeners: %" PRISIZE_T " | SyncRays: %3.1f AsyncRays: %3.1f",
		                      numActiveObjects, numObjects, numEvents, numEventListeners, numListeners, syncRays, asyncRays);

		posY += lineHeight;
		DrawATLComponentDebugInfo(*pAuxGeom, posX, posY);

		pAuxGeom->Commit(7);
	}
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::GetAudioTriggerData(ControlId const audioTriggerId, STriggerData& audioTriggerData) const
{
	CATLTrigger const* const pTrigger = stl::find_in_map(m_triggers, audioTriggerId, nullptr);

	if (pTrigger != nullptr)
	{
		audioTriggerData.radius = pTrigger->m_maxRadius;
		audioTriggerData.occlusionFadeOutDistance = pTrigger->m_occlusionFadeOutDistance;
	}
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::DrawATLComponentDebugInfo(IRenderAuxGeom& auxGeom, float posX, float const posY)
{
	m_fileCacheMgr.DrawDebugInfo(auxGeom, posX, posY);

	if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_ACTIVE_OBJECTS) > 0)
	{
		m_audioObjectMgr.DrawDebugInfo(auxGeom, posX, posY);
		posX += 300.0f;
	}

	if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_ACTIVE_EVENTS) > 0)
	{
		m_audioEventMgr.DrawDebugInfo(auxGeom, posX, posY);
		posX += 600.0f;
	}

	if ((g_audioCVars.m_drawAudioDebug & eADDF_SHOW_STANDALONE_FILES) > 0)
	{
		m_audioStandaloneFileMgr.DrawDebugInfo(auxGeom, posX, posY);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::RetriggerAudioControls()
{
	auto& registeredAudioObjects = m_audioObjectMgr.GetAudioObjects();
	for (auto pAudioObject : registeredAudioObjects)
	{
		pAudioObject->ForceImplementationRefresh(m_triggers, m_parameters, m_switches, m_environments, true);
	}

	m_pGlobalAudioObject->ForceImplementationRefresh(m_triggers, m_parameters, m_switches, m_environments, false);
}

///////////////////////////////////////////////////////////////////////////
void CAudioTranslationLayer::DrawAudioObjectDebugInfo(IRenderAuxGeom& auxGeom)
{
	m_audioObjectMgr.DrawPerObjectDebugInfo(
	  auxGeom,
	  m_audioListenerMgr.GetActiveListenerAttributes().transformation.GetPosition(),
	  m_triggers,
	  m_parameters,
	  m_switches,
	  m_preloadRequests,
	  m_environments);
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
