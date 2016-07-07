// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioProxy.h"
#include "AudioQueuedCommands.h"
#include "AudioCVars.h"
#include "AudioSystem.h"
#include <AudioLogger.h>
#include <CrySystem/ISystem.h>
#include <CryEntitySystem/IEntitySystem.h>

AudioControlId CAudioProxy::s_occlusionTypeSwitchId = INVALID_AUDIO_CONTROL_ID;
AudioSwitchStateId CAudioProxy::s_occlusionTypeStateIds[eAudioOcclusionType_Count] = {
	INVALID_AUDIO_SWITCH_STATE_ID,
	INVALID_AUDIO_SWITCH_STATE_ID,
	INVALID_AUDIO_SWITCH_STATE_ID,
	INVALID_AUDIO_SWITCH_STATE_ID,
	INVALID_AUDIO_SWITCH_STATE_ID,
	INVALID_AUDIO_SWITCH_STATE_ID
};

//////////////////////////////////////////////////////////////////////////
CAudioProxy::CAudioProxy()
	: m_audioObjectId(INVALID_AUDIO_OBJECT_ID)
	, m_flags(eAudioProxyFlags_None)
{
}

//////////////////////////////////////////////////////////////////////////
CAudioProxy::~CAudioProxy()
{
	CRY_ASSERT(m_audioObjectId == INVALID_AUDIO_OBJECT_ID);

	if (!m_queuedAudioCommands.empty())
	{
		for (auto const pExistingCommandBase : m_queuedAudioCommands)
		{
			POOL_FREE_CONST(pExistingCommandBase);
		}

		m_queuedAudioCommands.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::Initialize(char const* const szAudioObjectName, bool const bInitAsync /*= true*/)
{
	if ((bInitAsync && g_audioCVars.m_audioProxiesInitType == 0) || g_audioCVars.m_audioProxiesInitType == 2)
	{
		if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
		{
			m_flags |= eAudioProxyFlags_WaitingForId;

			SAudioRequest request;
			SAudioManagerRequestData<eAudioManagerRequestType_ReserveAudioObjectId> requestData(&m_audioObjectId, szAudioObjectName);
			request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_SyncCallback;
			request.pOwner = this;
			request.pData = &requestData;

			gEnv->pAudioSystem->PushRequest(request);
		}
		else
		{
			POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_Initialize>, pNewCommand)(szAudioObjectName);
			QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
		}
	}
	else
	{
		SAudioRequest request;
		SAudioManagerRequestData<eAudioManagerRequestType_ReserveAudioObjectId> requestData(&m_audioObjectId, szAudioObjectName);
		request.flags = eAudioRequestFlags_PriorityHigh | eAudioRequestFlags_ExecuteBlocking;
		request.pData = &requestData;

		gEnv->pAudioSystem->PushRequest(request);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		if (m_audioObjectId == INVALID_AUDIO_OBJECT_ID)
		{
			CryFatalError("<Audio> Failed to reserve audio object ID on AudioProxy (%s)!", szAudioObjectName);
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::ExecuteTrigger(
  AudioControlId const audioTriggerId,
  SAudioCallBackInfo const& callBackInfo /*= SAudioCallBackInfo::GetEmptyObject()*/)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		CRY_ASSERT(m_audioObjectId != INVALID_AUDIO_OBJECT_ID);

		SAudioRequest request;
		request.audioObjectId = m_audioObjectId;
		request.flags = callBackInfo.requestFlags;

		SAudioObjectRequestData<eAudioObjectRequestType_ExecuteTrigger> requestData(audioTriggerId, 0.0f);
		request.pOwner = (callBackInfo.pObjectToNotify != nullptr) ? callBackInfo.pObjectToNotify : this;
		request.pUserData = callBackInfo.pUserData;
		request.pUserDataOwner = callBackInfo.pUserDataOwner;
		request.pData = &requestData;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_ExecuteTrigger>, pNewCommand)(
		  audioTriggerId,
		  callBackInfo.pObjectToNotify,
		  callBackInfo.pUserData,
		  callBackInfo.pUserDataOwner,
		  callBackInfo.requestFlags);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::StopTrigger(AudioControlId const audioTriggerId)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_StopTrigger> requestData(audioTriggerId);
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_StopTrigger>, pNewCommand)(audioTriggerId);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetSwitchState(AudioControlId const audioSwitchId, AudioSwitchStateId const audioSwitchStateId)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_SetSwitchState> requestData(audioSwitchId, audioSwitchStateId);
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState>, pNewCommand)(audioSwitchId, audioSwitchStateId);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetRtpcValue(AudioControlId const audioRtpcId, float const value)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_SetRtpcValue> requestData(audioRtpcId, value);
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue>, pNewCommand)(audioRtpcId, value);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetOcclusionType(EAudioOcclusionType const occlusionType)
{
	AudioEnumFlagsType const index = static_cast<AudioEnumFlagsType>(occlusionType);

	if (index < eAudioOcclusionType_Count)
	{
		SetSwitchState(s_occlusionTypeSwitchId, s_occlusionTypeStateIds[index]);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetTransformation(Matrix34 const& transformation)
{
	// Update position only if the delta exceeds a given value.
	// Ideally this should be done on the caller's side!
	if (g_audioCVars.m_positionUpdateThreshold == 0.0f ||
	    !m_transformation.IsEquivalent(transformation, g_audioCVars.m_positionUpdateThreshold))
	{
		SetTransformationInternal(transformation);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetPosition(Vec3 const& position)
{
	// Update position only if the delta exceeds a given value.
	// Ideally this should be done on the caller's side!
	if (g_audioCVars.m_positionUpdateThreshold == 0.0f ||
	    !position.IsEquivalent(m_transformation.GetPosition(), g_audioCVars.m_positionUpdateThreshold))
	{
		SetTransformationInternal(Matrix34(IDENTITY, position));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetTransformationInternal(Matrix34 const& transformation)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		m_transformation = transformation;

		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_SetTransformation> requestData(transformation);
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation>, pNewCommand)(transformation);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetEnvironmentAmount(AudioEnvironmentId const audioEnvironmentId, float const amount)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_SetEnvironmentAmount> requestData(audioEnvironmentId, amount);
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount>, pNewCommand)(audioEnvironmentId, amount);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::SetCurrentEnvironments(EntityId const entityToIgnore /*= 0*/)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		ClearEnvironments();
		IAreaManager* const pAreaManager = gEnv->pEntitySystem->GetAreaManager();
		size_t numAreas = 0;
		static size_t const s_maxAreas = 10;
		static SAudioAreaInfo s_areaInfos[s_maxAreas];

		if (pAreaManager->QueryAudioAreas(m_transformation.GetPosition(), s_areaInfos, s_maxAreas, numAreas))
		{
			for (size_t i = 0; i < numAreas; ++i)
			{
				SAudioAreaInfo const& areaInfo = s_areaInfos[i];

				if (entityToIgnore == INVALID_ENTITYID || entityToIgnore != areaInfo.envProvidingEntityId)
				{
					SetEnvironmentAmount(areaInfo.audioEnvironmentId, areaInfo.amount);
				}
			}
		}
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_SetCurrentEnvironments>, pNewCommand)(entityToIgnore);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::ClearEnvironments()
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		SAudioRequest request;
		SAudioObjectRequestData<eAudioObjectRequestType_ResetEnvironments> requestData;
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;
		request.pData = &requestData;
		request.pOwner = this;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_ClearEnvironments>, pNewCommand);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::Reset()
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		if (m_audioObjectId != INVALID_AUDIO_OBJECT_ID)
		{
			// Request must be asynchronous and lowest priority!
			SAudioRequest request;
			SAudioObjectRequestData<eAudioObjectRequestType_ReleaseObject> requestData;
			request.audioObjectId = m_audioObjectId;
			request.pData = &requestData;

			gEnv->pAudioSystem->PushRequest(request);

			m_audioObjectId = INVALID_AUDIO_OBJECT_ID;
		}

		m_transformation = CAudioObjectTransformation();
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_Reset>, pNewCommand);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::Release()
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		Reset();
		static_cast<CAudioSystem*>(gEnv->pAudioSystem)->FreeAudioProxy(this);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_Release>, pNewCommand);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::PlayFile(SAudioPlayFileInfo const& playFileInfo, SAudioCallBackInfo const& callBackInfo /* = SAudioCallBackInfo::GetEmptyObject() */)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		CRY_ASSERT(m_audioObjectId != INVALID_AUDIO_OBJECT_ID);

		SAudioRequest request;
		request.audioObjectId = m_audioObjectId;
		request.flags = callBackInfo.requestFlags;
		SAudioObjectRequestData<eAudioObjectRequestType_PlayFile> requestData(playFileInfo.szFile, playFileInfo.bLocalized, playFileInfo.usedTriggerForPlayback);
		request.pOwner = (callBackInfo.pObjectToNotify != nullptr) ? callBackInfo.pObjectToNotify : this;
		request.pUserData = callBackInfo.pUserData;
		request.pUserDataOwner = callBackInfo.pUserDataOwner;
		request.pData = &requestData;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_PlayFile>, pNewCommand)(
		  playFileInfo.usedTriggerForPlayback,
		  callBackInfo.pObjectToNotify,
		  callBackInfo.pUserData,
		  callBackInfo.pUserDataOwner,
		  callBackInfo.requestFlags,
		  playFileInfo.bLocalized,
		  playFileInfo.szFile);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::StopFile(char const* const _szFile)
{
	if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
	{
		CRY_ASSERT(m_audioObjectId != INVALID_AUDIO_OBJECT_ID);

		SAudioRequest request;
		request.audioObjectId = m_audioObjectId;
		request.flags = eAudioRequestFlags_PriorityNormal;

		SAudioObjectRequestData<eAudioObjectRequestType_StopFile> requestData(_szFile);
		request.pData = &requestData;

		gEnv->pAudioSystem->PushRequest(request);
	}
	else
	{
		POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_StopFile>, pNewCommand)(_szFile);
		QueueCommand(static_cast<SQueuedAudioCommandBase const* const>(pNewCommand));
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::ExecuteQueuedCommands()
{
	m_flags &= ~eAudioProxyFlags_WaitingForId;

	if (!m_queuedAudioCommands.empty())
	{
		for (auto const pExistingCommandBase : m_queuedAudioCommands)
		{
			switch (pExistingCommandBase->type)
			{
			case eQueuedAudioCommandType_PlayFile:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_PlayFile> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_PlayFile> const* const>(pExistingCommandBase);
					SAudioCallBackInfo const callbackInfo(pExistingCommand->pOwnerOverride, pExistingCommand->pUserData, pExistingCommand->pUserDataOwner, pExistingCommand->requestFlags);
					SAudioPlayFileInfo const playbackInfo(pExistingCommand->fileName.c_str(), pExistingCommand->bLocalized, pExistingCommand->audioTriggerId);
					PlayFile(playbackInfo, callbackInfo);
				}
				break;
			case eQueuedAudioCommandType_StopFile:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_StopFile> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_StopFile> const* const>(pExistingCommandBase);
					StopFile(pExistingCommand->fileName.c_str());
				}
				break;
			case eQueuedAudioCommandType_ExecuteTrigger:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_ExecuteTrigger> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_ExecuteTrigger> const* const>(pExistingCommandBase);
					SAudioCallBackInfo const callbackInfo(pExistingCommand->pOwnerOverride, pExistingCommand->pUserData, pExistingCommand->pUserDataOwner, pExistingCommand->requestFlags);
					ExecuteTrigger(pExistingCommand->audioTriggerId, callbackInfo);
				}
				break;
			case eQueuedAudioCommandType_StopTrigger:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_StopTrigger> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_StopTrigger> const* const>(pExistingCommandBase);
					StopTrigger(pExistingCommand->audioTriggerId);
				}
				break;
			case eQueuedAudioCommandType_SetSwitchState:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const>(pExistingCommandBase);
					SetSwitchState(pExistingCommand->audioSwitchId, pExistingCommand->audioSwitchStateId);
				}
				break;
			case eQueuedAudioCommandType_SetRtpcValue:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const>(pExistingCommandBase);
					SetRtpcValue(pExistingCommand->audioRtpcId, pExistingCommand->value);
				}
				break;
			case eQueuedAudioCommandType_SetEnvironmentAmount:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const>(pExistingCommandBase);
					SetEnvironmentAmount(pExistingCommand->audioEnvironmentId, pExistingCommand->value);
				}
				break;
			case eQueuedAudioCommandType_SetCurrentEnvironments:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetCurrentEnvironments> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetCurrentEnvironments> const* const>(pExistingCommandBase);
					SetCurrentEnvironments(pExistingCommand->entityId);
				}
				break;
			case eQueuedAudioCommandType_ClearEnvironments:
				ClearEnvironments();
				break;
			case eQueuedAudioCommandType_SetTransformation:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation> const* const>(pExistingCommandBase);
					SetTransformationInternal(pExistingCommand->transformation);
				}
				break;
			case eQueuedAudioCommandType_Reset:
				Reset();
				break;
			case eQueuedAudioCommandType_Release:
				Release();
				break;
			case eQueuedAudioCommandType_Initialize:
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_Initialize> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_Initialize> const* const>(pExistingCommandBase);
					Initialize(pExistingCommand->audioObjectName.c_str(), true);
				}
				break;
			default:
				CryFatalError("Unknown command type in CAudioProxy::ExecuteQueuedCommands!");
				break;
			}

			if ((m_flags & eAudioProxyFlags_WaitingForId) > 0)
			{
				// An Initialize command was queued up.
				// Here we need to keep all commands after the Initialize.
				break;
			}
		}

		if ((m_flags & eAudioProxyFlags_WaitingForId) == 0)
		{
			for (auto const pExistingCommandBase : m_queuedAudioCommands)
			{
				POOL_FREE_CONST(pExistingCommandBase);
			}

			m_queuedAudioCommands.clear();
		}
		else
		{
			// An Initialize command was queued up.
			// Here we need to keep queued commands except for Reset and Initialize.
			TQueuedAudioCommands::iterator iter(m_queuedAudioCommands.begin());
			TQueuedAudioCommands::const_iterator iterEnd(m_queuedAudioCommands.cend());

			while (iter != iterEnd)
			{
				SQueuedAudioCommandBase const* const pExistingCommandBase = (*iter);

				if (pExistingCommandBase->type == eQueuedAudioCommandType_Reset || pExistingCommandBase->type == eQueuedAudioCommandType_Initialize)
				{
					POOL_FREE_CONST(pExistingCommandBase);
					iter = m_queuedAudioCommands.erase(iter);
					iterEnd = m_queuedAudioCommands.cend();
					continue;
				}

				++iter;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioProxy::QueueCommand(SQueuedAudioCommandBase const* const pNewCommandBase)
{
	bool bAdd = true;

	switch (pNewCommandBase->type)
	{
	case eQueuedAudioCommandType_PlayFile:
	case eQueuedAudioCommandType_StopFile:
	case eQueuedAudioCommandType_ExecuteTrigger:
	case eQueuedAudioCommandType_StopTrigger:
		// These type of commands get always pushed back!
		break;
	case eQueuedAudioCommandType_SetSwitchState:
		if (!m_queuedAudioCommands.empty())
		{
			SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const pNewCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const>(pNewCommandBase);
			std::replace_if(m_queuedAudioCommands.begin(), m_queuedAudioCommands.end(), [pNewCommand, &bAdd](SQueuedAudioCommandBase const* const pExistingCommandBase)
			{
				if (pExistingCommandBase->type == eQueuedAudioCommandType_SetSwitchState)
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetSwitchState> const* const>(pExistingCommandBase);

					if (pExistingCommand->audioSwitchId == pNewCommand->audioSwitchId)
					{
						POOL_FREE_CONST(pExistingCommand);
						bAdd = false;
						return true;
					}
				}

				return false;
			}, pNewCommand);

		}
		break;
	case eQueuedAudioCommandType_SetRtpcValue:
		if (!m_queuedAudioCommands.empty())
		{
			SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const pNewCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const>(pNewCommandBase);
			std::replace_if(m_queuedAudioCommands.begin(), m_queuedAudioCommands.end(), [pNewCommand, &bAdd](SQueuedAudioCommandBase const* const pExistingCommandBase)
			{
				if (pExistingCommandBase->type == eQueuedAudioCommandType_SetRtpcValue)
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetRtpcValue> const* const>(pExistingCommandBase);

					if (pExistingCommand->audioRtpcId == pNewCommand->audioRtpcId)
					{
						POOL_FREE_CONST(pExistingCommand);
						bAdd = false;
						return true;
					}
				}

				return false;
			}, pNewCommand);
		}
		break;
	case eQueuedAudioCommandType_SetTransformation:
		if (!m_queuedAudioCommands.empty())
		{
			SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation> const* const pNewCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetTransformation> const* const>(pNewCommandBase);
			std::replace_if(m_queuedAudioCommands.begin(), m_queuedAudioCommands.end(), [pNewCommand, &bAdd](SQueuedAudioCommandBase const* const pExistingCommandBase)
			{
				if (pExistingCommandBase->type == eQueuedAudioCommandType_SetTransformation)
				{
					POOL_FREE_CONST(pExistingCommandBase);
					bAdd = false;
					return true;
				}

				return false;
			}, pNewCommand);
		}
		break;
	case eQueuedAudioCommandType_SetEnvironmentAmount:
		if (!m_queuedAudioCommands.empty())
		{
			SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const pNewCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const>(pNewCommandBase);
			std::replace_if(m_queuedAudioCommands.begin(), m_queuedAudioCommands.end(), [pNewCommand, &bAdd](SQueuedAudioCommandBase const* const pExistingCommandBase)
			{
				if (pExistingCommandBase->type == eQueuedAudioCommandType_SetEnvironmentAmount)
				{
					SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const pExistingCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const>(pExistingCommandBase);

					if (pExistingCommand->audioEnvironmentId == pNewCommand->audioEnvironmentId)
					{
						POOL_FREE_CONST(pExistingCommand);
						bAdd = false;
						return true;
					}
				}

				return false;
			}, pNewCommand);
		}
		break;
	case eQueuedAudioCommandType_SetCurrentEnvironments:
	case eQueuedAudioCommandType_ClearEnvironments:
	case eQueuedAudioCommandType_Release:
		// These type of commands don't need another instance!
		if (!m_queuedAudioCommands.empty())
		{
			SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const pNewCommand = static_cast<SQueuedAudioCommand<eQueuedAudioCommandType_SetEnvironmentAmount> const* const>(pNewCommandBase);
			bAdd = std::find_if(m_queuedAudioCommands.begin(), m_queuedAudioCommands.end(), [](SQueuedAudioCommandBase const* const pExistingCommandBase)
			{
				return pExistingCommandBase->type == eQueuedAudioCommandType_SetEnvironmentAmount;
			}) == m_queuedAudioCommands.end();

			if (!bAdd)
			{
				POOL_FREE_CONST(pNewCommand);
			}
		}
		break;
	case eQueuedAudioCommandType_Reset:
		for (auto const pExistingCommandBase : m_queuedAudioCommands)
		{
			if (pExistingCommandBase->type == eQueuedAudioCommandType_Reset || pExistingCommandBase->type == eQueuedAudioCommandType_Release)
			{
				// If either eQACT_RESET or eQACT_RELEASE are already queued up then there is no need for adding a eQACT_RESET command.
				bAdd = false;

				// Free memory of unused new command.
				POOL_FREE_CONST(pNewCommandBase);

				break;
			}
		}
		break;
	case eQueuedAudioCommandType_Initialize:
		{
			// There must be only 1 Initialize command be queued up.
			m_queuedAudioCommands.clear();

			// Precede the Initialization with a Reset command to release the pending audio object.
			POOL_NEW_CREATE(SQueuedAudioCommand<eQueuedAudioCommandType_Reset>, pNewCommand);
			m_queuedAudioCommands.push_back(pNewCommand);
		}
		break;
	default:
		{
			CRY_ASSERT(false);
			g_audioLogger.Log(eAudioLogType_Error, "Unknown queued command type in CAudioProxy::TryAddQueuedCommand!");
			bAdd = false;

			// Free memory of unused new command.
			POOL_FREE_CONST(pNewCommandBase);
		}
		break;
	}

	if (bAdd)
	{
		if (pNewCommandBase->type != eQueuedAudioCommandType_SetTransformation)
		{
			m_queuedAudioCommands.push_back(pNewCommandBase);
		}
		else
		{
			// Make sure we set position first!
			m_queuedAudioCommands.push_front(pNewCommandBase);
		}
	}
}
