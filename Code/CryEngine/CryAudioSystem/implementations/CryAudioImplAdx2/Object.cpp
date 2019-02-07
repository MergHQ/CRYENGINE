// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Object.h"

#include "CueInstance.h"
#include "Listener.h"
#include "Cvars.h"

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	#include <Logger.h>
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
constexpr CriChar8 const* g_szOcclusionAisacName = "occlusion";

//////////////////////////////////////////////////////////////////////////
CObject::CObject(CTransformation const& transformation)
	: m_transformation(transformation)
	, m_occlusion(0.0f)
	, m_previousAbsoluteVelocity(0.0f)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	, m_absoluteVelocity(0.0f)
	, m_absoluteVelocityNormalized(0.0f)
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
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
void CObject::SetOcclusion(float const occlusion)
{
	criAtomExPlayer_SetAisacControlByName(m_pPlayer, g_szOcclusionAisacName, static_cast<CriFloat32>(occlusion));
	criAtomExPlayer_UpdateAll(m_pPlayer);

	m_occlusion = occlusion;
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)

	if (((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0) || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		bool isVirtual = false;
		// To do: add check for virtual states.

		if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				Debug::g_objectFontSize,
				isVirtual ? Debug::s_globalColorVirtual : Debug::s_objectColorParameter,
				false,
				"[Adx2] %s: %2.2f m/s (%2.2f)\n",
				static_cast<char const*>(g_szAbsoluteVelocityAisacName),
				m_absoluteVelocity,
				m_absoluteVelocityNormalized);

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
				"[Adx2] Doppler calculation enabled\n");
		}
	}

#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

/////////////////////////////////////////////////////////////////////////
void CObject::UpdateVelocityTracking()
{
	bool trackAbsoluteVelocity = false;
	bool trackDoppler = false;

	CueInstances::iterator iter = m_cueInstances.begin();
	CueInstances::const_iterator iterEnd = m_cueInstances.end();

	while ((iter != iterEnd) && !(trackAbsoluteVelocity && trackDoppler))
	{
		CCueInstance* const pCueInstance = *iter;
		trackAbsoluteVelocity |= ((pCueInstance->GetFlags() & ECueInstanceFlags::HasAbsoluteVelocity) != 0);
		trackDoppler |= ((pCueInstance->GetFlags() & ECueInstanceFlags::HasDoppler) != 0);
		++iter;
	}

	if (trackAbsoluteVelocity)
	{
		if (g_cvars.m_maxVelocity > 0.0f)
		{
			m_flags |= EObjectFlags::TrackAbsoluteVelocity;
		}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Adx2 - Cannot enable absolute velocity tracking, because s_Adx2MaxVelocity is not greater than 0.");
		}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}
	else
	{
		m_flags &= ~EObjectFlags::TrackAbsoluteVelocity;

		criAtomExPlayer_SetAisacControlByName(m_pPlayer, g_szAbsoluteVelocityAisacName, 0.0f);
		criAtomExPlayer_UpdateAll(m_pPlayer);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		m_absoluteVelocity = 0.0f;
		m_absoluteVelocityNormalized = 0.0f;
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}

	if (trackDoppler)
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

			CriAtomExVector const zeroVelocity{ 0.0f, 0.0f, 0.0f };
			criAtomEx3dSource_SetVelocity(m_p3dSource, &zeroVelocity);
			criAtomEx3dSource_Update(m_p3dSource);

			CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists during %s", __FUNCTION__);
			g_numObjectsWithDoppler--;
		}
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

			criAtomExPlayer_SetAisacControlByName(m_pPlayer, g_szAbsoluteVelocityAisacName, static_cast<CriFloat32>(absoluteVelocityNormalized));
			criAtomExPlayer_UpdateAll(m_pPlayer);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
			m_absoluteVelocity = absoluteVelocity;
			m_absoluteVelocityNormalized = absoluteVelocityNormalized;
#endif      // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
		}
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
