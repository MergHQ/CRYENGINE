// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include <PoolObject.h>
#include <ATLEntities.h>
#include <IAudioImpl.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{

AkGameObjectID SAudioObject::s_dummyGameObjectId = static_cast<AkGameObjectID>(-2);

// AK callbacks
void EndEventCallback(AkCallbackType callbackType, AkCallbackInfo* pCallbackInfo)
{
	if (callbackType == AK_EndOfEvent)
	{
		SAudioEvent* const pAudioEvent = static_cast<SAudioEvent* const>(pCallbackInfo->pCookie);

		if (pAudioEvent != nullptr)
		{
			SRequestUserData const data(eRequestFlags_ThreadSafePush);
			gEnv->pAudioSystem->ReportFinishedEvent(pAudioEvent->atlEvent, true, data);
		}
	}
}

void PrepareEventCallback(
  AkUniqueID eventId,
  void const* pBankPtr,
  AKRESULT wwiseResult,
  AkMemPoolId memPoolId,
  void* pCookie)
{
	SAudioEvent* const pAudioEvent = static_cast<SAudioEvent* const>(pCookie);

	if (pAudioEvent != nullptr)
	{
		pAudioEvent->id = eventId;
		SRequestUserData const data(eRequestFlags_ThreadSafePush);
		gEnv->pAudioSystem->ReportFinishedEvent(pAudioEvent->atlEvent, wwiseResult == AK_Success, data);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::Update()
{
	ERequestStatus result = eRequestStatus_Failure;

	if (bNeedsToUpdateEnvironments)
	{
		result = PostEnvironmentAmounts();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::Set3DAttributes(SObject3DAttributes const& attributes)
{
	AkSoundPosition soundPos;
	FillAKObjectPosition(attributes.transformation, soundPos);

	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(id, soundPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Wwise SetPosition failed with AKRESULT: %d", wwiseResult);
		return eRequestStatus_Failure;
	}

	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	static float const envEpsilon = 0.0001f;

	ERequestStatus result = eRequestStatus_Failure;

	SAudioEnvironment const* const pEnvironment = static_cast<SAudioEnvironment const* const>(pIAudioEnvironment);

	if (pEnvironment != nullptr)
	{
		switch (pEnvironment->type)
		{
		case eWwiseAudioEnvironmentType_AuxBus:
			{
				float const currentAmount = stl::find_in_map(environemntImplAmounts, pEnvironment->busId, -1.0f);

				if ((currentAmount == -1.0f) || (fabs(currentAmount - amount) > envEpsilon))
				{
					environemntImplAmounts[pEnvironment->busId] = amount;
					bNeedsToUpdateEnvironments = true;
				}

				result = eRequestStatus_Success;
				break;
			}
		case eWwiseAudioEnvironmentType_Rtpc:
			{
				AkRtpcValue rtpcValue = static_cast<AkRtpcValue>(pEnvironment->multiplier * amount + pEnvironment->shift);

				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pEnvironment->rtpcId, rtpcValue, id);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = eRequestStatus_Success;
				}
				else
				{
					g_audioImplLogger.Log(
					  eAudioLogType_Warning,
					  "Wwise failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
					  pEnvironment->rtpcId,
					  rtpcValue,
					  id);
				}
				break;

			}
		default:
			{
				CRY_ASSERT(false); //Unknown AudioEnvironmentImplementation type
			}
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Invalid EnvironmentData passed to the Wwise implementation of SetEnvironment");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetParameter(IParameter const* const pAudioRtpc, float const value)
{
	ERequestStatus result = eRequestStatus_Failure;
	SAudioRtpc const* const pAKRtpcData = static_cast<SAudioRtpc const* const>(pAudioRtpc);
	if (pAKRtpcData != nullptr)
	{
		AkRtpcValue rtpcValue = static_cast<AkRtpcValue>(pAKRtpcData->mult * value + pAKRtpcData->shift);

		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pAKRtpcData->id, rtpcValue, id);

		if (IS_WWISE_OK(wwiseResult))
		{
			result = eRequestStatus_Success;
		}
		else
		{
			g_audioImplLogger.Log(
			  eAudioLogType_Warning,
			  "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
			  pAKRtpcData->id,
			  static_cast<AkRtpcValue>(value),
			  id);
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Invalid RtpcData passed to the Wwise implementation of SetParameter");
	}

	return result;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	ERequestStatus result = eRequestStatus_Failure;

	SAudioSwitchState const* const pSwitchState = static_cast<SAudioSwitchState const* const>(pIAudioSwitchState);

	if (pSwitchState != nullptr)
	{
		switch (pSwitchState->type)
		{
		case eWwiseSwitchType_Switch:
			{
				AkGameObjectID const gameObjectId = id != AK_INVALID_GAME_OBJECT ? id : s_dummyGameObjectId;

				AKRESULT const wwiseResult = AK::SoundEngine::SetSwitch(
				  pSwitchState->switchId,
				  pSwitchState->stateId,
				  gameObjectId);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = eRequestStatus_Success;
				}
				else
				{
					g_audioImplLogger.Log(
					  eAudioLogType_Warning,
					  "Wwise failed to set the switch group %" PRISIZE_T " to state %" PRISIZE_T " on object %" PRISIZE_T,
					  pSwitchState->switchId,
					  pSwitchState->stateId,
					  gameObjectId);
				}

				break;
			}
		case eWwiseSwitchType_State:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetState(
				  pSwitchState->switchId,
				  pSwitchState->stateId);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = eRequestStatus_Success;
				}
				else
				{
					g_audioImplLogger.Log(
					  eAudioLogType_Warning,
					  "Wwise failed to set the state group %" PRISIZE_T "to state %" PRISIZE_T,
					  pSwitchState->switchId,
					  pSwitchState->stateId);
				}

				break;
			}
		case eWwiseSwitchType_Rtpc:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(
				  pSwitchState->switchId,
				  static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
				  id);

				if (IS_WWISE_OK(wwiseResult))
				{
					result = eRequestStatus_Success;
				}
				else
				{
					g_audioImplLogger.Log(
					  eAudioLogType_Warning,
					  "Wwise failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
					  pSwitchState->switchId,
					  static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
					  id);
				}

				break;
			}
		case eWwiseSwitchType_None:
			{
				break;
			}
		default:
			{
				g_audioImplLogger.Log(eAudioLogType_Warning, "Unknown EWwiseSwitchType: %" PRISIZE_T, pSwitchState->type);

				break;
			}
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Invalid SwitchState passed to the Wwise implementation of SetSwitchState");
	}

	return result;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	ERequestStatus result = eRequestStatus_Failure;

	AKRESULT const wwiseResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
	  id,
	  0,                                // only set the obstruction/occlusion for the default listener for now
	  static_cast<AkReal32>(occlusion), // Currently used on obstruction until the ATL produces a correct obstruction value.
	  static_cast<AkReal32>(occlusion));

	if (IS_WWISE_OK(wwiseResult))
	{
		result = eRequestStatus_Success;
	}
	else
	{
		g_audioImplLogger.Log(
		  eAudioLogType_Warning,
		  "Wwise failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
		  obstruction,
		  occlusion,
		  id);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent)
{

	ERequestStatus result = eRequestStatus_Failure;

	SAudioTrigger const* const pAudioTrigger = static_cast<SAudioTrigger const* const>(pIAudioTrigger);
	SAudioEvent* const pAudioEvent = static_cast<SAudioEvent*>(pIAudioEvent);

	if ((pAudioTrigger != nullptr) && (pAudioEvent != nullptr))
	{
		AkGameObjectID gameObjectId = AK_INVALID_GAME_OBJECT;

		if (id != AK_INVALID_GAME_OBJECT)
		{
			gameObjectId = id;
			PostEnvironmentAmounts();
		}
		else
		{
			// If ID is invalid, then it is the global audio object
			gameObjectId = SAudioObject::s_dummyGameObjectId;
		}

		AkPlayingID const id = AK::SoundEngine::PostEvent(pAudioTrigger->id, gameObjectId, AK_EndOfEvent, &EndEventCallback, pAudioEvent);

		if (id != AK_INVALID_PLAYING_ID)
		{
			pAudioEvent->audioEventState = eAudioEventState_Playing;
			pAudioEvent->id = id;
			result = eRequestStatus_Success;
		}
		else
		{
			// if posting an Event failed, try to prepare it, if it isn't prepared already
			g_audioImplLogger.Log(eAudioLogType_Warning, "Failed to Post Wwise event %" PRISIZE_T, pAudioEvent->id);
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ExecuteTrigger.");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::StopAllTriggers()
{
	AkGameObjectID const gameObjectId = id != AK_INVALID_GAME_OBJECT ? id : SAudioObject::s_dummyGameObjectId;
	AK::SoundEngine::StopAll(gameObjectId);
	return eRequestStatus_Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioObject::PostEnvironmentAmounts()
{

	ERequestStatus result = eRequestStatus_Failure;
	AkAuxSendValue auxValues[AK_MAX_AUX_PER_OBJ];
	uint32 auxIndex = 0;

	SAudioObject::EnvironmentImplMap::iterator iEnvPair = environemntImplAmounts.begin();
	SAudioObject::EnvironmentImplMap::const_iterator const iEnvStart = environemntImplAmounts.begin();
	SAudioObject::EnvironmentImplMap::const_iterator const iEnvEnd = environemntImplAmounts.end();

	if (environemntImplAmounts.size() <= AK_MAX_AUX_PER_OBJ)
	{
		for (; iEnvPair != iEnvEnd; ++auxIndex)
		{
			float const fAmount = iEnvPair->second;

			auxValues[auxIndex].auxBusID = iEnvPair->first;
			auxValues[auxIndex].fControlValue = fAmount;

			// If an amount is zero, we still want to send it to the middleware, but we also want to remove it from the map.
			if (fAmount == 0.0f)
			{
				environemntImplAmounts.erase(iEnvPair++);
			}
			else
			{
				++iEnvPair;
			}
		}
	}
	else
	{
		// sort the environments in order of decreasing amounts and take the first AK_MAX_AUX_PER_OBJ worth
		typedef std::set<std::pair<AkAuxBusID, float>, SEnvPairCompare> TEnvPairSet;
		TEnvPairSet cEnvPairs(iEnvStart, iEnvEnd);

		TEnvPairSet::const_iterator iSortedEnvPair = cEnvPairs.begin();
		TEnvPairSet::const_iterator const iSortedEnvEnd = cEnvPairs.end();

		for (; (iSortedEnvPair != iSortedEnvEnd) && (auxIndex < AK_MAX_AUX_PER_OBJ); ++iSortedEnvPair, ++auxIndex)
		{
			auxValues[auxIndex].auxBusID = iSortedEnvPair->first;
			auxValues[auxIndex].fControlValue = iSortedEnvPair->second;
		}

		//remove all Environments with 0.0 amounts
		while (iEnvPair != iEnvEnd)
		{
			if (iEnvPair->second == 0.0f)
			{
				environemntImplAmounts.erase(iEnvPair++);
			}
			else
			{
				++iEnvPair;
			}
		}
	}

	CRY_ASSERT(auxIndex <= AK_MAX_AUX_PER_OBJ);

	AKRESULT const wwiseResult = AK::SoundEngine::SetGameObjectAuxSendValues(id, auxValues, auxIndex);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = eRequestStatus_Success;
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Wwise SetGameObjectAuxSendValues failed on object %" PRISIZE_T " with AKRESULT: %d", id, wwiseResult);
	}

	bNeedsToUpdateEnvironments = false;

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioEvent::Stop()
{

	switch (audioEventState)
	{
	case eAudioEventState_Playing:
		{
			AK::SoundEngine::StopPlayingID(id, 10);
			break;
		}
	default:
		{
			// Stopping an event of this type is not supported!
			CRY_ASSERT(false);
			break;
		}
	}

	return eRequestStatus_Success;
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioListener::Set3DAttributes(SObject3DAttributes const& attributes)
{
	ERequestStatus result = eRequestStatus_Failure;
	AkListenerPosition listenerPos;
	FillAKListenerPosition(attributes.transformation, listenerPos);
	AKRESULT const wwiseResult = AK::SoundEngine::SetListenerPosition(listenerPos, id);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = eRequestStatus_Success;
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Warning, "Wwise SetListenerPosition failed with AKRESULT: %" PRISIZE_T, wwiseResult);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::Load() const
{
	return SetLoaded(true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::Unload() const
{
	return SetLoaded(false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::LoadAsync(IAudioEvent* const pIAudioEvent) const
{
	return SetLoadedAsync(pIAudioEvent, true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::UnloadAsync(IAudioEvent* const pIAudioEvent) const
{
	return SetLoadedAsync(pIAudioEvent, false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::SetLoaded(bool bLoad) const
{
	ERequestStatus result = eRequestStatus_Failure;

	AkUniqueID nImplAKID = id;

	AKRESULT const wwiseResult =
	  AK::SoundEngine::PrepareEvent(bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload, &nImplAKID, 1);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = eRequestStatus_Success;
	}
	else
	{
		g_audioImplLogger.Log(
		  eAudioLogType_Warning,
		  "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %" PRISIZE_T,
		  bLoad ? "Preparation_Load" : "Preparation_Unload",
		  id,
		  wwiseResult);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus SAudioTrigger::SetLoadedAsync(IAudioEvent* const pIAudioEvent, bool bLoad) const
{
	ERequestStatus result = eRequestStatus_Failure;

	SAudioEvent* const pAudioEvent = static_cast<SAudioEvent*>(pIAudioEvent);

	if (pAudioEvent != nullptr)
	{
		AkUniqueID nImplAKID = id;
		AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(
		  bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
		  &nImplAKID,
		  1,
		  &PrepareEventCallback,
		  pAudioEvent);

		if (IS_WWISE_OK(wwiseResult))
		{
			pAudioEvent->id = id;
			pAudioEvent->audioEventState = eAudioEventState_Unloading;

			result = eRequestStatus_Success;
		}
		else
		{
			g_audioImplLogger.Log(
			  eAudioLogType_Warning,
			  "Wwise PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
			  bLoad ? "Preparation_Load" : "Preparation_Unload",
			  id,
			  wwiseResult);
		}
	}
	else
	{
		g_audioImplLogger.Log(eAudioLogType_Error,
		                      "Invalid IAudioEvent passed to the Wwise implementation of %sTriggerAsync",
		                      bLoad ? "Load" : "Unprepare");
	}

	return result;
}

}
}
}
