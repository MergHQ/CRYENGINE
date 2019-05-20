// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"
#include "CVars.h"
#include "Event.h"
#include "EventInstance.h"
#include "Listener.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	#include <DebugStyle.h>
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CObject::CObject(CTransformation const& transformation, int const listenerMask, Listeners const& listeners)
	: CBaseObject(listenerMask, listeners)
	, m_transformation(transformation)
	, m_previousAbsoluteVelocity(0.0f)
	, m_lowestOcclusionPerListener(1.0f)
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
	CBaseObject::Update(deltaTime);

	if (((m_flags& EObjectFlags::MovingOrDecaying) != 0) && (deltaTime > 0.0f))
	{
		UpdateVelocities(deltaTime);
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

	float const threshold = GetDistanceToListener() * g_cvars.m_positionUpdateThresholdMultiplier;

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
void CObject::SetOcclusion(IListener* const pIListener, float const occlusion, uint8 const numRemainingListeners)
{
	// Lowest occlusion value of all listeners is used.
	m_lowestOcclusionPerListener = std::min(m_lowestOcclusionPerListener, occlusion);

	if (numRemainingListeners == 1)
	{
		SetParameter(g_occlusionParameterInfo, m_lowestOcclusionPerListener);

		for (auto const pEventInstance : m_eventInstances)
		{
			pEventInstance->SetOcclusion(m_lowestOcclusionPerListener);
		}

		m_occlusion = m_lowestOcclusionPerListener;
		m_lowestOcclusionPerListener = 1.0f;
	}
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
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		bool isVirtual = true;

		for (auto const pEventInstance : m_eventInstances)
		{
			if (pEventInstance->GetState() != EEventState::Virtual)
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
				Debug::g_objectFontSize,
				isVirtual ? Debug::s_globalColorVirtual : Debug::s_objectColorParameter,
				false,
				"[Fmod] %s: %2.2f m/s\n",
				g_szAbsoluteVelocityParameterName,
				m_absoluteVelocity);

			posY += Debug::g_objectLineHeight;
		}

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				Debug::g_objectFontSize,
				isVirtual ? Debug::s_globalColorVirtual : Debug::s_objectColorActive,
				false,
				"[Fmod] Doppler calculation enabled\n");
		}
	}

#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::Set3DAttributes()
{
	for (auto const pEventInstance : m_eventInstances)
	{
		FMOD_RESULT const fmodResult = pEventInstance->GetFmodEventInstance()->set3DAttributes(&m_attributes);
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
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
			SetParameter(g_absoluteVelocityParameterInfo, absoluteVelocity);
			m_absoluteVelocity = absoluteVelocity;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
float CObject::GetDistanceToListener()
{
	float shortestDistanceToListener = std::numeric_limits<float>::max();

	for (auto const pListener : m_listeners)
	{
		float const distance = m_position.GetDistance(pListener->GetPosition());
		shortestDistanceToListener = std::min(shortestDistanceToListener, distance);
	}

	return shortestDistanceToListener;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio