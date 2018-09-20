// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Listener.h"
#include "Common.h"

#include <Logger.h>
#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
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
	AkListenerPosition listenerPos;
	FillAKListenerPosition(transformation, listenerPos);
	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(id, listenerPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise - CListener constructor failed with AKRESULT: %d", wwiseResult);
	}
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

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	// Always update velocity in non-release builds for debug draw.
	m_isMovingOrDecaying = true;
#else
	if (g_numObjectsWithRelativeVelocity > 0)
	{
		m_isMovingOrDecaying = true;
	}
	else
	{
		m_previousPosition = m_position;
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

	AKRESULT const wwiseResult = AK::SoundEngine::SetPosition(m_id, listenerPos);

	if (!IS_WWISE_OK(wwiseResult))
	{
		Cry::Audio::Log(ELogType::Warning, "Wwise - CListener::SetTransformation failed with AKRESULT: %d", wwiseResult);
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
