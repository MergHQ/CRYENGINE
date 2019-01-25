// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "BaseObject.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

#include <AK/SoundEngine/Common/AkSoundEngine.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
//////////////////////////////////////////////////////////////////////////
void CParameter::Set(IObject* const pIObject, float const value)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
		auto const rtpcValue = static_cast<AkRtpcValue>(m_multiplier * value + m_shift);

		AK::SoundEngine::SetRTPCValue(m_id, rtpcValue, pBaseObject->GetId());
	}
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid object passed to the Wwise implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CParameter::SetGlobally(float const value)
{
	auto const rtpcValue = static_cast<AkRtpcValue>(m_multiplier * value + m_shift);

	AK::SoundEngine::SetRTPCValue(m_id, rtpcValue, AK_INVALID_GAME_OBJECT);
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
