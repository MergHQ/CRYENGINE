// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SwitchState.h"
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
//////////////////////////////////////////////////////////////////////////
void CSwitchState::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject const*>(pIObject);

		switch (m_type)
		{
		case ESwitchType::StateGroup:
			{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				AKRESULT const wwiseResult = AK::SoundEngine::SetState(m_stateOrSwitchGroupId, m_stateOrSwitchId);

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise failed to set the StateGroup %" PRISIZE_T "to state %" PRISIZE_T,
						m_stateOrSwitchGroupId,
						m_stateOrSwitchId);
				}
#else
				AK::SoundEngine::SetState(m_stateOrSwitchGroupId, m_stateOrSwitchId);
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
				break;
			}
		case ESwitchType::SwitchGroup:
			{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				AKRESULT const wwiseResult = AK::SoundEngine::SetSwitch(m_stateOrSwitchGroupId, m_stateOrSwitchId, pObject->GetId());

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the SwitchGroup %" PRISIZE_T " to state %" PRISIZE_T " on object %s",
						m_stateOrSwitchGroupId,
						m_stateOrSwitchId,
						pObject->GetName());
				}
#else
				AK::SoundEngine::SetSwitch(m_stateOrSwitchGroupId, m_stateOrSwitchId, pObject->GetId());
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

				break;
			}
		case ESwitchType::Rtpc:
			{
#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(m_stateOrSwitchGroupId, static_cast<AkRtpcValue>(m_rtpcValue), pObject->GetId());

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the Rtpc %" PRISIZE_T " to value %f on object %s",
						m_stateOrSwitchGroupId,
						static_cast<AkRtpcValue>(m_rtpcValue),
						pObject->GetName());
				}
#else
				AK::SoundEngine::SetRTPCValue(m_stateOrSwitchGroupId, static_cast<AkRtpcValue>(m_rtpcValue), pObject->GetId());
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE

				break;
			}
		case ESwitchType::None:
			{
				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Wwise - Unknown ESwitchType: %" PRISIZE_T, m_type);

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid SwitchState passed to the Wwise implementation of SetSwitchState");
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
