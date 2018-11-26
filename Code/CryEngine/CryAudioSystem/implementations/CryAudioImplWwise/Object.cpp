// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "Common.h"
#include "CVars.h"
#include "Environment.h"
#include "Event.h"
#include "Listener.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Trigger.h"

#include <Logger.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

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
CObject::CObject(AkGameObjectID const id, CTransformation const& transformation, char const* const szName)
	: m_id(id)
	, m_needsToUpdateEnvironments(false)
	, m_needsToUpdateVirtualStates(false)
	, m_flags(EObjectFlags::None)
	, m_distanceToListener(0.0f)
	, m_previousRelativeVelocity(0.0f)
	, m_previousAbsoluteVelocity(0.0f)
	, m_transformation(transformation)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	, m_name(szName)
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
{
	m_auxSendValues.reserve(4);

	AkSoundPosition soundPos;
	FillAKObjectPosition(transformation, soundPos);
	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(id, soundPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise - CObject constructor failed with AKRESULT: %d", wwiseResult);
	}
}

//////////////////////////////////////////////////////////////////////////
CObject::~CObject()
{
	if ((m_flags& EObjectFlags::TrackRelativeVelocity) != 0)
	{
		CRY_ASSERT_MESSAGE(g_numObjectsWithRelativeVelocity > 0, "g_numObjectsWithRelativeVelocity is 0 but an object with relative velocity tracking still exists during %s", __FUNCTION__);
		g_numObjectsWithRelativeVelocity--;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	SetDistanceToListener();

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
				if (!pEvent->m_toBeRemoved)
				{
					pEvent->UpdateVirtualState(m_distanceToListener);
				}
			}
		}
	}

	if (deltaTime > 0.0f)
	{
		UpdateVelocities(deltaTime);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CTransformation const& transformation)
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
void CObject::SetOcclusion(float const occlusion)
{
	if (m_id != g_globalObjectId)
	{
		if (g_listenerId != AK_INVALID_GAME_OBJECT)
		{
			AKRESULT const wwiseResult = AK::SoundEngine::SetObjectObstructionAndOcclusion(
				m_id,
				g_listenerId,                     // Set occlusion for only the default listener for now.
				static_cast<AkReal32>(occlusion), // The occlusion value is currently used on obstruction as well until a correct obstruction value is calculated.
				static_cast<AkReal32>(occlusion));

			if (!IS_WWISE_OK(wwiseResult))
			{
				Cry::Audio::Log(
					ELogType::Warning,
					"Wwise - failed to set Obstruction %f and Occlusion %f on object %" PRISIZE_T,
					occlusion,
					occlusion,
					m_id);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Wwise - invalid listener Id during %s!", __FUNCTION__);
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Trying to set occlusion value on the global object!");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
{
	if (m_id != g_globalObjectId)
	{
		// For disabling ray casts of the propagation processor if an object is virtual.
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
void CObject::AddEvent(CEvent* const pEvent)
{
	m_events.push_back(pEvent);
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
ERequestStatus CObject::SetName(char const* const szName)
{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	StopAllTriggers();

	AKRESULT wwiseResult = AK::SoundEngine::UnregisterGameObj(m_id);
	CRY_ASSERT(wwiseResult == AK_Success);

	wwiseResult = AK::SoundEngine::RegisterGameObj(m_id, szName);
	CRY_ASSERT(wwiseResult == AK_Success);

	m_name = szName;

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

//////////////////////////////////////////////////////////////////////////
void CObject::SetDistanceToListener()
{
	m_distanceToListener = m_position.GetDistance(g_pListener->GetPosition());
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

						CRY_ASSERT_MESSAGE(g_numObjectsWithRelativeVelocity > 0, "g_numObjectsWithRelativeVelocity is 0 but an object with relative velocity tracking still exists during %s", __FUNCTION__);
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
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
