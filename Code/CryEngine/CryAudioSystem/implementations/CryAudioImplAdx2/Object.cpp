// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"

#include "Event.h"
#include "Environment.h"
#include "Listener.h"
#include "Cvars.h"

#include <Logger.h>

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
static constexpr CriChar8 const* s_szOcclusionAisacName = "occlusion";

//////////////////////////////////////////////////////////////////////////
CObject::CObject(CTransformation const& transformation)
	: m_transformation(transformation)
	, m_occlusion(0.0f)
	, m_previousAbsoluteVelocity(0.0f)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
{
	Fill3DAttributeTransformation(transformation, m_3dAttributes);
	criAtomEx3dSource_SetPosition(m_p3dSource, &m_3dAttributes.pos);
	criAtomEx3dSource_SetOrientation(m_p3dSource, &m_3dAttributes.fwd, &m_3dAttributes.up);
	criAtomEx3dSource_Update(m_p3dSource);
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
		Fill3DAttributeTransformation(transformation, m_3dAttributes);

		criAtomEx3dSource_SetPosition(m_p3dSource, &m_3dAttributes.pos);
		criAtomEx3dSource_SetOrientation(m_p3dSource, &m_3dAttributes.fwd, &m_3dAttributes.up);

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			Fill3DAttributeVelocity(m_velocity, m_3dAttributes);
			criAtomEx3dSource_SetVelocity(m_p3dSource, &m_3dAttributes.vel);
		}

		criAtomEx3dSource_Update(m_p3dSource);
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	auto const pEnvironment = static_cast<CEnvironment const*>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		if (pEnvironment->GetType() == EEnvironmentType::Bus)
		{
			criAtomExPlayer_SetBusSendLevelByName(m_pPlayer, pEnvironment->GetName(), static_cast<CriFloat32>(amount));
			criAtomExPlayer_UpdateAll(m_pPlayer);
		}
		else
		{
			criAtomExPlayer_SetAisacControlByName(m_pPlayer, pEnvironment->GetName(), static_cast<CriFloat32>(pEnvironment->GetMultiplier() * amount + pEnvironment->GetShift()));
			criAtomExPlayer_UpdateAll(m_pPlayer);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusion(float const occlusion)
{
	criAtomExPlayer_SetAisacControlByName(m_pPlayer, s_szOcclusionAisacName, static_cast<CriFloat32>(occlusion));
	criAtomExPlayer_UpdateAll(m_pPlayer);

	m_occlusion = occlusion;
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		bool isVirtual = false;
		// To do: add check for virtual states.

		if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				g_debugObjectFontSize,
				isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
				false,
				"[Adx2] %s: %2.2f m/s (%2.2f)\n",
				static_cast<char const*>(s_szAbsoluteVelocityAisacName),
				m_absoluteVelocity,
				m_absoluteVelocityNormalized);

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
				"[Adx2] Doppler calculation enabled\n");
		}
	}

#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
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
			Fill3DAttributeVelocity(m_velocity, m_3dAttributes);
			criAtomEx3dSource_SetVelocity(m_p3dSource, &m_3dAttributes.vel);
			criAtomEx3dSource_Update(m_p3dSource);
		}
	}

	if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
	{
		float const absoluteVelocity = m_velocity.GetLength();

		if (absoluteVelocity == 0.0f || fabs(absoluteVelocity - m_previousAbsoluteVelocity) > g_cvars.m_velocityTrackingThreshold)
		{
			m_previousAbsoluteVelocity = absoluteVelocity;
			float const absoluteVelocityNormalized = (std::min(absoluteVelocity, g_cvars.m_maxVelocity) / g_cvars.m_maxVelocity);

			criAtomExPlayer_SetAisacControlByName(m_pPlayer, s_szAbsoluteVelocityAisacName, static_cast<CriFloat32>(absoluteVelocityNormalized));
			criAtomExPlayer_UpdateAll(m_pPlayer);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
			m_absoluteVelocity = absoluteVelocity;
			m_absoluteVelocityNormalized = absoluteVelocityNormalized;
#endif      // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
