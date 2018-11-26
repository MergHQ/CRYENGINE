// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Parameter.h"
#include "Object.h"
#include "Common.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif  // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
void CParameter::Set(IObject* const pIObject, float const value)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject const*>(pIObject);
		auto const rtpcValue = static_cast<AkRtpcValue>(m_multiplier * value + m_shift);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
		AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(m_id, rtpcValue, pObject->GetId());

		if (!IS_WWISE_OK(wwiseResult))
		{
			Cry::Audio::Log(
				ELogType::Warning,
				"Wwise - failed to set the Rtpc %" PRISIZE_T " to value %f on object %s",
				m_id,
				static_cast<AkRtpcValue>(value),
				pObject->GetName());
		}
#else
		AK::SoundEngine::SetRTPCValue(m_id, rtpcValue, pObject->GetId());
#endif    // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
