// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ATLEntities.h"
#include "AudioImplCVars.h"
#include <Logger.h>

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
// AK callbacks
void EndEventCallback(AkCallbackType callbackType, AkCallbackInfo* pCallbackInfo)
{
	if (callbackType == AK_EndOfEvent)
	{
		CATLEvent* const pEvent = static_cast<CATLEvent* const>(pCallbackInfo->pCookie);

		if (pEvent != nullptr)
		{
			gEnv->pAudioSystem->ReportFinishedEvent(*pEvent, true);
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
	CEvent* const pEvent = static_cast<CEvent* const>(pCookie);

	if (pEvent != nullptr)
	{
		pEvent->m_id = eventId;
		gEnv->pAudioSystem->ReportFinishedEvent(pEvent->m_atlEvent, wwiseResult == AK_Success);
	}
}

static constexpr char const* s_szAbsoluteVelocityParameterName = "absolute_velocity";
static AkRtpcID const s_absoluteVelocityParameterId = AK::SoundEngine::GetIDFromString(s_szAbsoluteVelocityParameterName);

static constexpr char const* s_szRelativeVelocityParameterName = "relative_velocity";
static AkRtpcID const s_relativeVelocityParameterId = AK::SoundEngine::GetIDFromString(s_szRelativeVelocityParameterName);

//////////////////////////////////////////////////////////////////////////
void SetParameterById(AkRtpcID const rtpcId, AkRtpcValue const value, AkGameObjectID const objectId)
{
	AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(rtpcId, value, objectId);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(
			ELogType::Warning,
			"Wwise - failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
			rtpcId,
			value,
			objectId);
	}
}

//////////////////////////////////////////////////////////////////////////
CObject::CObject(AkGameObjectID const id, CObjectTransformation const& transformation)
	: m_id(id)
	, m_needsToUpdateEnvironments(false)
	, m_needsToUpdateVirtualStates(false)
	, m_flags(EObjectFlags::None)
	, m_distanceToListener(0.0f)
	, m_previousRelativeVelocity(0.0f)
	, m_previousAbsoluteVelocity(0.0f)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
{
	m_auxSendValues.reserve(4);
}

//////////////////////////////////////////////////////////////////////////
CObject::~CObject()
{
	if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
	{
		CRY_ASSERT_MESSAGE(g_numObjectsWithRelativeVelocity > 0, "g_numObjectsWithRelativeVelocity is 0 but an object with relative velocity tracking still exists.");
		g_numObjectsWithRelativeVelocity--;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	m_distanceToListener = m_position.GetDistance(g_pListener->GetPosition());

	if (m_needsToUpdateEnvironments)
	{
		PostEnvironmentAmounts();
	}

#if !defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	// Always update in production code for debug draw.
	if ((m_flags& EObjectFlags::UpdateVirtualStates) != 0)
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	{
		if (m_needsToUpdateVirtualStates)
		{
			for (auto const pEvent : m_events)
			{
				pEvent->UpdateVirtualState(m_distanceToListener);
			}
		}
	}

	if (deltaTime > 0.0f)
	{
		UpdateVelocities(deltaTime);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CObjectTransformation const& transformation)
{
	m_position = transformation.GetPosition();

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0))
	{
		m_flags |= EObjectFlags::MovingOrDecaying;
	}
	else
	{
		m_previousPosition = m_position;
	}

	float const threshold = m_distanceToListener * g_cvars.m_positionUpdateThresholdMultiplier;

	if (!m_transformation.IsEquivalent(transformation, threshold))
	{
		m_transformation = transformation;

		AkSoundPosition soundPos;
		FillAKObjectPosition(m_transformation, soundPos);
		AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(m_id, soundPos);

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(ELogType::Warning, "Wwise - CObject::SetTransformation failed with AKRESULT: %d", wwiseResult);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	static float const envEpsilon = 0.0001f;
	SEnvironment const* const pEnvironment = static_cast<SEnvironment const* const>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		switch (pEnvironment->type)
		{
		case EEnvironmentType::AuxBus:
			{
				if (m_id != g_globalObjectId)
				{
					bool addAuxSendValue = true;

					for (auto& auxSendValue : m_auxSendValues)
					{
						if (auxSendValue.auxBusID == pEnvironment->busId)
						{
							addAuxSendValue = false;

							if (fabs(auxSendValue.fControlValue - amount) > envEpsilon)
							{
								auxSendValue.fControlValue = amount;
								m_needsToUpdateEnvironments = true;
							}

							break;
						}
					}

					if (addAuxSendValue)
					{
						// This temporary copy is needed until AK equips AkAuxSendValue with a ctor.
						m_auxSendValues.emplace_back(AkAuxSendValue{ g_listenerId, pEnvironment->busId, amount });
						m_needsToUpdateEnvironments = true;
					}
				}
				else
				{
					Cry::Audio::Log(ELogType::Error, "Trying to set an environment on the global object!");
				}

				break;
			}
		case EEnvironmentType::Rtpc:
			{
				AkRtpcValue const rtpcValue = static_cast<AkRtpcValue>(pEnvironment->multiplier * amount + pEnvironment->shift);
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pEnvironment->rtpcId, rtpcValue, m_id);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the Rtpc %u to value %f on object %u in SetEnvironement()",
						pEnvironment->rtpcId,
						rtpcValue,
						m_id);
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
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid EnvironmentData passed to the Wwise implementation of SetEnvironment");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	SParameter const* const pParameter = static_cast<SParameter const* const>(pIParameter);

	if (pParameter != nullptr)
	{
		AkRtpcValue rtpcValue = static_cast<AkRtpcValue>(pParameter->mult * value + pParameter->shift);

		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(pParameter->id, rtpcValue, m_id);

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
				pParameter->id,
				static_cast<AkRtpcValue>(value),
				m_id);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid RtpcData passed to the Wwise implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	SSwitchState const* const pSwitchState = static_cast<SSwitchState const* const>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		switch (pSwitchState->type)
		{
		case ESwitchType::StateGroup:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetState(
					pSwitchState->stateOrSwitchGroupId,
					pSwitchState->stateOrSwitchId);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise failed to set the StateGroup %" PRISIZE_T "to state %" PRISIZE_T,
						pSwitchState->stateOrSwitchGroupId,
						pSwitchState->stateOrSwitchId);
				}

				break;
			}
		case ESwitchType::SwitchGroup:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetSwitch(
					pSwitchState->stateOrSwitchGroupId,
					pSwitchState->stateOrSwitchId,
					m_id);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the SwitchGroup %" PRISIZE_T " to state %" PRISIZE_T " on object %" PRISIZE_T,
						pSwitchState->stateOrSwitchGroupId,
						pSwitchState->stateOrSwitchId,
						m_id);
				}

				break;
			}
		case ESwitchType::Rtpc:
			{
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(
					pSwitchState->stateOrSwitchGroupId,
					static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
					m_id);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the Rtpc %" PRISIZE_T " to value %f on object %" PRISIZE_T,
						pSwitchState->stateOrSwitchGroupId,
						static_cast<AkRtpcValue>(pSwitchState->rtpcValue),
						m_id);
				}

				break;
			}
		case ESwitchType::None:
			{
				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Wwise - Unknown ESwitchType: %" PRISIZE_T, pSwitchState->type);

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid SwitchState passed to the Wwise implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	if (m_id != g_globalObjectId)
	{
		if (g_listenerId != AK_INVALID_GAME_OBJECT)
		{
			AKRESULT const wwiseResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
				m_id,
				g_listenerId,                     // Set obstruction/occlusion for only the default listener for now.
				static_cast<AkReal32>(occlusion), // The occlusion value is currently used on obstruction as well until a correct obstruction value is calculated.
				static_cast<AkReal32>(occlusion));

			if (!IS_WWISE_OK(wwiseResult))
			{
				Cry::Audio::Log(
					ELogType::Warning,
					"Wwise - failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
					obstruction,
					occlusion,
					m_id);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Wwise - invalid listener Id during SetObjectObstructionAndOcclusion!");
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
{
	if (m_id != g_globalObjectId)
	{
		if ((occlusionType != EOcclusionType::None) && (occlusionType != EOcclusionType::Ignore))
		{
			m_flags |= EObjectFlags::UpdateVirtualStates;
		}
		else
		{
			m_flags &= ~EObjectFlags::UpdateVirtualStates;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent)
{
	ERequestStatus result = ERequestStatus::Failure;

	CTrigger const* const pTrigger = static_cast<CTrigger const* const>(pITrigger);
	CEvent* const pEvent = static_cast<CEvent* const>(pIEvent);

	if ((pTrigger != nullptr) && (pEvent != nullptr))
	{
		// If the user executes a trigger on the global object we want to post events only to that particular object and not globally!
		AkGameObjectID objectId = g_globalObjectId;

		if (m_id != AK_INVALID_GAME_OBJECT)
		{
			objectId = m_id;
			PostEnvironmentAmounts();
		}

		AkPlayingID const id = AK::SoundEngine::PostEvent(pTrigger->m_id, objectId, AK_EndOfEvent, &EndEventCallback, &pEvent->m_atlEvent);

		if (id != AK_INVALID_PLAYING_ID)
		{
			pEvent->m_id = id;
			pEvent->m_pObject = this;
			pEvent->m_maxAttenuation = pTrigger->m_maxAttenuation;

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
			// Always update in production code for debug draw.
			pEvent->UpdateVirtualState(m_distanceToListener);
#else
			if ((m_flags& EObjectFlags::UpdateVirtualStates) != 0)
			{
				pEvent->UpdateVirtualState(m_distanceToListener);
			}
			else
			{
				pEvent->m_state = EEventState::Playing;
			}
#endif      // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

			m_events.push_back(pEvent);
			m_needsToUpdateVirtualStates = (m_id != g_globalObjectId);

			result = ERequestStatus::Success;
		}
		else
		{
			// if posting an Event failed, try to prepare it, if it isn't prepared already
			Cry::Audio::Log(ELogType::Warning, "Failed to Post Wwise event %" PRISIZE_T, pEvent->m_id);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Wwise implementation of ExecuteTrigger.");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CObject::RemoveEvent(CEvent* const pEvent)
{
	if (!stl::find_and_erase(m_events, pEvent))
	{
		Cry::Audio::Log(ELogType::Error, "Tried to remove an event from an object that does not own that event");
	}

	m_needsToUpdateVirtualStates = ((m_id != g_globalObjectId) && !m_events.empty());
}

//////////////////////////////////////////////////////////////////////////
void CObject::StopAllTriggers()
{
	// If the user wants to stop all triggers on the global object we want to stop them only on that particular object and not globally!
	AkGameObjectID const objectId = (m_id != AK_INVALID_GAME_OBJECT) ? m_id : g_globalObjectId;
	AK::SoundEngine::StopAll(objectId);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::PlayFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::StopFile(IStandaloneFile* const pIStandaloneFile)
{
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	StopAllTriggers();

	AKRESULT wwiseResult = AK::SoundEngine::UnregisterGameObj(m_id);
	CRY_ASSERT(wwiseResult == AK_Success);

	wwiseResult = AK::SoundEngine::RegisterGameObj(m_id, szName);
	CRY_ASSERT(wwiseResult == AK_Success);

	return ERequestStatus::SuccessNeedsRefresh;
#else
	return ERequestStatus::Success;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::PostEnvironmentAmounts()
{
	std::size_t const numEnvironments = m_auxSendValues.size();

	if (numEnvironments > 0)
	{
		AKRESULT const wwiseResult = AK::SoundEngine::SetGameObjectAuxSendValues(m_id, &m_auxSendValues[0], static_cast<AkUInt32>(numEnvironments));

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(ELogType::Warning, "Wwise - SetGameObjectAuxSendValues failed on object %" PRISIZE_T " with AKRESULT: %d", m_id, wwiseResult);
		}

		m_auxSendValues.erase(
			std::remove_if(
				m_auxSendValues.begin(),
				m_auxSendValues.end(),
				[](AkAuxSendValue const& auxSendValue) -> bool { return auxSendValue.fControlValue == 0.0f; }
				),
			m_auxSendValues.end()
			);
	}

	m_needsToUpdateEnvironments = false;
}

///////////////////////////////////////////////////////////////////////////
void CObject::UpdateVelocities(float const deltaTime)
{
	if ((m_flags& EObjectFlags::MovingOrDecaying) != 0)
	{
		Vec3 const deltaPos(m_position - m_previousPosition);

		if (!deltaPos.IsZero())
		{
			m_velocity = deltaPos / deltaTime;
			m_previousPosition = m_position;
		}
		else if (!m_velocity.IsZero())
		{
			// We did not move last frame, begin exponential decay towards zero.
			float const decay = std::max(1.0f - deltaTime / 0.05f, 0.0f);
			m_velocity *= decay;

			if (m_velocity.GetLengthSquared() < FloatEpsilon)
			{
				m_velocity = ZERO;
				m_flags &= ~EObjectFlags::MovingOrDecaying;
			}
		}

		if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
		{
			float const absoluteVelocity = m_velocity.GetLength();

			if (absoluteVelocity == 0.0f || fabs(absoluteVelocity - m_previousAbsoluteVelocity) > g_cvars.m_velocityTrackingThreshold)
			{
				m_previousAbsoluteVelocity = absoluteVelocity;

				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(s_absoluteVelocityParameterId, static_cast<AkRtpcValue>(absoluteVelocity), m_id);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the %s parameter to value %f on object %" PRISIZE_T,
						s_szAbsoluteVelocityParameterName,
						absoluteVelocity,
						m_id);
				}
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				else
				{
					m_parameterInfo[s_szAbsoluteVelocityParameterName] = absoluteVelocity;
				}
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
			}
		}

		if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
		{
			// Approaching positive, departing negative value.
			float relativeVelocity = 0.0f;

			if (((m_flags& EObjectFlags::MovingOrDecaying) != 0) && !g_pListener->HasMoved())
			{
				relativeVelocity = -m_velocity.Dot((m_position - g_pListener->GetPosition()).GetNormalized());
			}
			else if (((m_flags& EObjectFlags::MovingOrDecaying) != 0) && g_pListener->HasMoved())
			{
				Vec3 const relativeVelocityVec(m_velocity - g_pListener->GetVelocity());
				relativeVelocity = -relativeVelocityVec.Dot((m_position - g_pListener->GetPosition()).GetNormalized());
			}

			TryToSetRelativeVelocity(relativeVelocity);
		}
	}
	else if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
	{
		// Approaching positive, departing negative value.
		if (g_pListener->HasMoved())
		{
			float const relativeVelocity = g_pListener->GetVelocity().Dot((m_position - g_pListener->GetPosition()).GetNormalized());
			TryToSetRelativeVelocity(relativeVelocity);
		}
		else if (m_previousRelativeVelocity != 0.0f)
		{
			TryToSetRelativeVelocity(0.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::TryToSetRelativeVelocity(float const relativeVelocity)
{
	if (relativeVelocity == 0.0f || fabs(relativeVelocity - m_previousRelativeVelocity) > g_cvars.m_velocityTrackingThreshold)
	{
		m_previousRelativeVelocity = relativeVelocity;

		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(s_relativeVelocityParameterId, static_cast<AkRtpcValue>(relativeVelocity), m_id);

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - failed to set the %s parameter to value %f on object %" PRISIZE_T,
				s_szRelativeVelocityParameterName,
				relativeVelocity,
				m_id);
		}
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
		else
		{
			m_parameterInfo[s_szRelativeVelocityParameterName] = relativeVelocity;
		}
#endif    // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
	if (m_id != g_globalObjectId)
	{
		switch (type)
		{
		case EObjectFunctionality::TrackAbsoluteVelocity:
			{
				if (enable)
				{
					m_flags |= EObjectFlags::TrackAbsoluteVelocity;
				}
				else
				{
					m_flags &= ~EObjectFlags::TrackAbsoluteVelocity;

					SetParameterById(s_absoluteVelocityParameterId, 0.0f, m_id);
				}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				if (enable)
				{
					m_parameterInfo[s_szAbsoluteVelocityParameterName] = 0.0f;
				}
				else
				{
					m_parameterInfo.erase(s_szAbsoluteVelocityParameterName);
				}
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

				break;
			}
		case EObjectFunctionality::TrackRelativeVelocity:
			{
				if (enable)
				{
					if ((m_flags& EObjectFlags::TrackRelativeVelocity) == 0)
					{
						m_flags |= EObjectFlags::TrackRelativeVelocity;
						g_numObjectsWithRelativeVelocity++;
					}
				}
				else
				{
					if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
					{
						m_flags &= ~EObjectFlags::TrackRelativeVelocity;
						SetParameterById(s_relativeVelocityParameterId, 0.0f, m_id);

						CRY_ASSERT_MESSAGE(g_numObjectsWithRelativeVelocity > 0, "g_numObjectsWithRelativeVelocity is 0 but an object with relative velocity tracking still exists.");
						g_numObjectsWithRelativeVelocity--;
					}
				}

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				if (enable)
				{
					m_parameterInfo[s_szRelativeVelocityParameterName] = 0.0f;
				}
				else
				{
					m_parameterInfo.erase(s_szRelativeVelocityParameterName);
				}
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

				break;
			}
		default:
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)

	if (!m_parameterInfo.empty())
	{
		bool isVirtual = true;

		for (auto const pEvent : m_events)
		{
			if (pEvent->m_state != EEventState::Virtual)
			{
				isVirtual = false;
				break;
			}
		}

		for (auto const& parameterPair : m_parameterInfo)
		{
			bool canDraw = true;

			if (szTextFilter != nullptr)
			{
				CryFixedStringT<MaxControlNameLength> lowerCaseParameterName(parameterPair.first);
				lowerCaseParameterName.MakeLower();

				if (lowerCaseParameterName.find(szTextFilter) == CryFixedStringT<MaxControlNameLength>::npos)
				{
					canDraw = false;
				}
			}

			if (canDraw)
			{
				auxGeom.Draw2dLabel(
					posX,
					posY,
					g_debugObjectFontSize,
					isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
					false,
					"[Wwise] %s: %2.2f\n",
					parameterPair.first,
					parameterPair.second);

				posY += g_debugObjectLineHeight;
			}
		}
	}

#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Stop()
{
	switch (m_state)
	{
	case EEventState::Playing:
	case EEventState::Virtual:
		{
			AK::SoundEngine::StopPlayingID(m_id, 10);
			break;
		}
	default:
		{
			// Stopping an event of this type is not supported!
			CRY_ASSERT(false);
			break;
		}
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
CEvent::~CEvent()
{
	if (m_pObject != nullptr)
	{
		m_pObject->RemoveEvent(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEvent::UpdateVirtualState(float const distance)
{
	EEventState const state = ((m_maxAttenuation > 0.0f) && (m_maxAttenuation < distance)) ? EEventState::Virtual : EEventState::Playing;

	if (m_state != state)
	{
		m_state = state;

		if (m_state == EEventState::Virtual)
		{
			gEnv->pAudioSystem->ReportVirtualizedEvent(m_atlEvent);
		}
		else
		{
			gEnv->pAudioSystem->ReportPhysicalizedEvent(m_atlEvent);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CListener::CListener(CObjectTransformation const& transformation, AkGameObjectID const id)
	: m_id(id)
	, m_transformation(transformation)
	, m_hasMoved(false)
	, m_isMovingOrDecaying(false)
	, m_velocity(ZERO)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
{
}

//////////////////////////////////////////////////////////////////////////
void CListener::Update(float const deltaTime)
{
	if (m_isMovingOrDecaying && (deltaTime > 0.0f))
	{
		m_hasMoved = m_velocity.GetLengthSquared() > FloatEpsilon;
		Vec3 const deltaPos(m_position - m_previousPosition);

		if (!deltaPos.IsZero())
		{
			m_velocity = deltaPos / deltaTime;
			m_previousPosition = m_position;
		}
		else if (!m_velocity.IsZero())
		{
			// We did not move last frame, begin exponential decay towards zero.
			float const decay = std::max(1.0f - deltaTime / 0.05f, 0.0f);
			m_velocity *= decay;

			if (m_velocity.GetLengthSquared() < FloatEpsilon)
			{
				m_velocity = ZERO;
				m_isMovingOrDecaying = false;
			}
		}
		else
		{
			m_isMovingOrDecaying = false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetName(char const* const szName)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetTransformation(CObjectTransformation const& transformation)
{
	m_transformation = transformation;
	m_position = transformation.GetPosition();

	AkListenerPosition listenerPos;
	FillAKListenerPosition(transformation, listenerPos);

	if (g_numObjectsWithRelativeVelocity > 0)
	{
		m_isMovingOrDecaying = true;
	}
	else
	{
		m_previousPosition = m_position;
	}

	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(m_id, listenerPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise - CListener::SetTransformation failed with AKRESULT: %d", wwiseResult);
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Load() const
{
	return SetLoaded(true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Unload() const
{
	return SetLoaded(false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::LoadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, true);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::UnloadAsync(IEvent* const pIEvent) const
{
	return SetLoadedAsync(pIEvent, false);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoaded(bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;
	AkUniqueID id = m_id;
	AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload, &id, 1);

	if (IS_WWISE_OK(wwiseResult))
	{
		result = ERequestStatus::Success;
	}
	else
	{
		Cry::Audio::Log(
			ELogType::Warning,
			"Wwise - PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
			bLoad ? "Preparation_Load" : "Preparation_Unload",
			m_id,
			wwiseResult);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::SetLoadedAsync(IEvent* const pIEvent, bool const bLoad) const
{
	ERequestStatus result = ERequestStatus::Failure;

	CEvent* const pEvent = static_cast<CEvent*>(pIEvent);

	if (pEvent != nullptr)
	{
		AkUniqueID id = m_id;
		AKRESULT const wwiseResult = AK::SoundEngine::PrepareEvent(
			bLoad ? AK::SoundEngine::Preparation_Load : AK::SoundEngine::Preparation_Unload,
			&id,
			1,
			&PrepareEventCallback,
			pEvent);

		if (IS_WWISE_OK(wwiseResult))
		{
			pEvent->m_id = m_id;
			pEvent->m_state = EEventState::Unloading;
			result = ERequestStatus::Success;
		}
		else
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - PrepareEvent with %s failed for Wwise event %" PRISIZE_T " with AKRESULT: %d",
				bLoad ? "Preparation_Load" : "Preparation_Unload",
				m_id,
				wwiseResult);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error,
		                "Wwise - Invalid IEvent passed to the Wwise implementation of %sTriggerAsync",
		                bLoad ? "Load" : "Unprepare");
	}

	return result;
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
