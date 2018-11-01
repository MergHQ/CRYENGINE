// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "BaseStandaloneFile.h"
#include "CVars.h"
#include "Environment.h"
#include "Event.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Listener.h"
#include "Trigger.h"
#include "VcaParameter.h"
#include "VcaState.h"

#include <Logger.h>

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CObject::CObject(CTransformation const& transformation)
	: m_transformation(transformation)
	, m_previousAbsoluteVelocity(0.0f)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
{
	Fill3DAttributeTransformation(transformation, m_attributes);
	Set3DAttributes();
}

//////////////////////////////////////////////////////////////////////////
CObject::~CObject()
{
	if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
	{
		CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists during %s", __FUNCTION__);
		g_numObjectsWithDoppler--;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	if (((m_flags& EObjectFlags::MovingOrDecaying) != 0) && (deltaTime > 0.0f))
	{
		UpdateVelocities(deltaTime);
	}

	CBaseObject::Update(deltaTime);

#if !defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	// Always update in production code for debug draw.
	if ((m_flags& EObjectFlags::UpdateVirtualStates) != 0)
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	{
		for (auto const pEvent : m_events)
		{
			pEvent->UpdateVirtualState();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CTransformation const& transformation)
{
	m_position = transformation.GetPosition();

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		m_flags |= EObjectFlags::MovingOrDecaying;
	}
	else
	{
		m_previousPosition = m_position;
	}

	float const threshold = m_position.GetDistance(g_pListener->GetPosition()) * g_cvars.m_positionUpdateThresholdMultiplier;

	if (!m_transformation.IsEquivalent(transformation, threshold))
	{
		m_transformation = transformation;
		Fill3DAttributeTransformation(m_transformation, m_attributes);

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			Fill3DAttributeVelocity(m_velocity, m_attributes);
		}

		Set3DAttributes();
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	auto const pEnvironment = static_cast<CEnvironment const*>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		bool bSet = true;
		auto const iter(m_environments.find(pEnvironment));

		if (iter != m_environments.end())
		{
			if (bSet = (fabs(iter->second - amount) > 0.001f))
			{
				iter->second = amount;
			}
		}
		else
		{
			m_environments.emplace(pEnvironment, amount);
		}

		if (bSet)
		{
			for (auto const pEvent : m_events)
			{
				pEvent->TrySetEnvironment(pEnvironment, amount);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid IEnvironment pointer passed to the Fmod implementation of SetEnvironment");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	auto const pBaseParameter = static_cast<CBaseParameter const*>(pIParameter);

	if (pBaseParameter != nullptr)
	{
		EParameterType const type = pBaseParameter->GetType();

		if (type == EParameterType::Parameter)
		{
			auto const pParameter = static_cast<CParameter const*>(pBaseParameter);
			uint32 const parameterId = pParameter->GetId();
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

			for (auto const pEvent : m_events)
			{
				FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
				CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist during %s", __FUNCTION__);
				CTrigger const* const pTrigger = pEvent->GetTrigger();
				CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist during %s", __FUNCTION__);

				FMOD::Studio::EventDescription* pEventDescription = nullptr;
				fmodResult = pEventInstance->getDescription(&pEventDescription);
				ASSERT_FMOD_OK;

				if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
				{
					ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

					if (parameters.find(parameterId) != parameters.end())
					{
						fmodResult = pEventInstance->setParameterValueByIndex(parameters[parameterId], pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
						ASSERT_FMOD_OK;
					}
					else
					{
						int parameterCount = 0;
						fmodResult = pEventInstance->getParameterCount(&parameterCount);
						ASSERT_FMOD_OK;

						for (int index = 0; index < parameterCount; ++index)
						{
							FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
							fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
							ASSERT_FMOD_OK;

							if (parameterId == StringToId(parameterDescription.name))
							{
								parameters.emplace(parameterId, index);
								fmodResult = pEventInstance->setParameterValueByIndex(index, pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
								ASSERT_FMOD_OK;
								break;
							}
						}
					}
				}
				else
				{
					int parameterCount = 0;
					fmodResult = pEventInstance->getParameterCount(&parameterCount);
					ASSERT_FMOD_OK;

					for (int index = 0; index < parameterCount; ++index)
					{
						FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
						fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
						ASSERT_FMOD_OK;

						if (parameterId == StringToId(parameterDescription.name))
						{
							g_triggerToParameterIndexes[pTrigger].emplace(std::make_pair(parameterId, index));
							fmodResult = pEventInstance->setParameterValueByIndex(index, pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
							ASSERT_FMOD_OK;
							break;
						}
					}
				}
			}

			auto const iter(m_parameters.find(pParameter));

			if (iter != m_parameters.end())
			{
				iter->second = value;
			}
			else
			{
				m_parameters.emplace(pParameter, value);
			}
		}
		else if (type == EParameterType::VCA)
		{
			auto const pVca = static_cast<CVcaParameter const*>(pBaseParameter);
			FMOD_RESULT const fmodResult = pVca->GetVca()->setVolume(pVca->GetValueMultiplier() * value + pVca->GetValueShift());
			ASSERT_FMOD_OK;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid AudioObjectData or ParameterData passed to the Fmod implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	auto const pBaseSwitchState = static_cast<CBaseSwitchState const*>(pISwitchState);

	if (pBaseSwitchState != nullptr)
	{
		EStateType const type = pBaseSwitchState->GetType();

		if (type == EStateType::State)
		{
			auto const pSwitchState = static_cast<CSwitchState const*>(pBaseSwitchState);
			uint32 const parameterId = pSwitchState->GetId();
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

			for (auto const pEvent : m_events)
			{
				FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
				CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist during %s", __FUNCTION__);
				CTrigger const* const pTrigger = pEvent->GetTrigger();
				CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist during %s", __FUNCTION__);

				FMOD::Studio::EventDescription* pEventDescription = nullptr;
				fmodResult = pEventInstance->getDescription(&pEventDescription);
				ASSERT_FMOD_OK;

				if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
				{
					ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

					if (parameters.find(parameterId) != parameters.end())
					{
						fmodResult = pEventInstance->setParameterValueByIndex(parameters[parameterId], pSwitchState->GetValue());
						ASSERT_FMOD_OK;
					}
					else
					{
						int parameterCount = 0;
						fmodResult = pEventInstance->getParameterCount(&parameterCount);
						ASSERT_FMOD_OK;

						for (int index = 0; index < parameterCount; ++index)
						{
							FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
							fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
							ASSERT_FMOD_OK;

							if (parameterId == StringToId(parameterDescription.name))
							{
								parameters.emplace(parameterId, index);
								fmodResult = pEventInstance->setParameterValueByIndex(index, pSwitchState->GetValue());
								ASSERT_FMOD_OK;
								break;
							}
						}
					}
				}
				else
				{
					int parameterCount = 0;
					fmodResult = pEventInstance->getParameterCount(&parameterCount);
					ASSERT_FMOD_OK;

					for (int index = 0; index < parameterCount; ++index)
					{
						FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
						fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
						ASSERT_FMOD_OK;

						if (parameterId == StringToId(parameterDescription.name))
						{
							g_triggerToParameterIndexes[pTrigger].emplace(std::make_pair(parameterId, index));
							fmodResult = pEventInstance->setParameterValueByIndex(index, pSwitchState->GetValue());
							ASSERT_FMOD_OK;
							break;
						}
					}
				}
			}

			auto const iter(m_switches.find(pSwitchState->GetId()));

			if (iter != m_switches.end())
			{
				iter->second = pSwitchState;
			}
			else
			{
				m_switches.emplace(pSwitchState->GetId(), pSwitchState);
			}
		}
		else if (type == EStateType::VCA)
		{
			auto const pVca = static_cast<CVcaState const*>(pBaseSwitchState);
			FMOD_RESULT const fmodResult = pVca->GetVca()->setVolume(pVca->GetValue());
			ASSERT_FMOD_OK;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Fmod implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusion(float const occlusion)
{
	for (auto const pEvent : m_events)
	{
		pEvent->SetOcclusion(occlusion);
	}

	m_occlusion = occlusion;
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
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

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
	switch (type)
	{
	case EObjectFunctionality::TrackRelativeVelocity:
		{
			if (enable)
			{
				if ((m_flags& EObjectFlags::TrackVelocityForDoppler) == 0)
				{
					m_flags |= EObjectFlags::TrackVelocityForDoppler;
					g_numObjectsWithDoppler++;
				}
			}
			else
			{
				if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
				{
					m_flags &= ~EObjectFlags::TrackVelocityForDoppler;

					Vec3 const zeroVelocity{ 0.0f, 0.0f, 0.0f };
					Fill3DAttributeVelocity(zeroVelocity, m_attributes);
					Set3DAttributes();

					CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists during %s", __FUNCTION__);
					g_numObjectsWithDoppler--;
				}
			}

			break;
		}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		bool isVirtual = true;

		for (auto const pEvent : m_events)
		{
			if (pEvent->GetState() != EEventState::Virtual)
			{
				isVirtual = false;
				break;
			}
		}

		if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				g_debugObjectFontSize,
				isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
				false,
				"[Fmod] %s: %2.2f m/s\n",
				s_szAbsoluteVelocityParameterName,
				m_absoluteVelocity);

			posY += g_debugObjectLineHeight;
		}

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				g_debugObjectFontSize,
				isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
				false,
				"[Fmod] Doppler calculation enabled\n");
		}
	}

#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::Set3DAttributes()
{
	for (auto const pEvent : m_events)
	{
		FMOD_RESULT const fmodResult = pEvent->GetInstance()->set3DAttributes(&m_attributes);
		ASSERT_FMOD_OK;
	}

	for (auto const pFile : m_files)
	{
		pFile->Set3DAttributes(m_attributes);
	}
}

///////////////////////////////////////////////////////////////////////////
void CObject::UpdateVelocities(float const deltaTime)
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

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			Fill3DAttributeVelocity(m_velocity, m_attributes);
			Set3DAttributes();
		}
	}

	if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
	{
		float const absoluteVelocity = m_velocity.GetLength();

		if (absoluteVelocity == 0.0f || fabs(absoluteVelocity - m_previousAbsoluteVelocity) > g_cvars.m_velocityTrackingThreshold)
		{
			m_previousAbsoluteVelocity = absoluteVelocity;
			SetAbsoluteVelocity(absoluteVelocity);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetAbsoluteVelocity(float const velocity)
{
	for (auto const pEvent : m_events)
	{
		pEvent->SetAbsoluteVelocity(velocity);
	}

	m_absoluteVelocity = velocity;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio