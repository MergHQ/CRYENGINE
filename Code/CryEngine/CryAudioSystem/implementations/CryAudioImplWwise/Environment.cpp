// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Environment.h"
#include "Object.h"
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
CEnvironment::CEnvironment(EEnvironmentType const type, AkAuxBusID const busId)
	: m_type(type)
	, m_busId(busId)
{
	CRY_ASSERT(type == EEnvironmentType::AuxBus);
}

//////////////////////////////////////////////////////////////////////////
CEnvironment::CEnvironment(
	EEnvironmentType const type,
	AkRtpcID const rtpcId,
	float const multiplier,
	float const shift)
	: m_type(type)
	, m_rtpcId(rtpcId)
	, m_multiplier(multiplier)
	, m_shift(shift)
{
	CRY_ASSERT(type == EEnvironmentType::Rtpc);
}

//////////////////////////////////////////////////////////////////////////
void CEnvironment::Set(IObject* const pIObject, float const amount)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CObject*>(pIObject);

		switch (m_type)
		{
		case EEnvironmentType::AuxBus:
			{
				if (pObject->GetId() != g_globalObjectId)
				{
					static float const envEpsilon = 0.0001f;
					bool addAuxSendValue = true;
					auto& auxSendValues = pObject->GetAuxSendValues();

					for (auto& auxSendValue : auxSendValues)
					{
						if (auxSendValue.auxBusID == m_busId)
						{
							addAuxSendValue = false;

							if (fabs(auxSendValue.fControlValue - amount) > envEpsilon)
							{
								auxSendValue.fControlValue = amount;
								pObject->SetNeedsToUpdateEnvironments(true);
							}

							break;
						}
					}

					if (addAuxSendValue)
					{
						// This temporary copy is needed until AK equips AkAuxSendValue with a ctor.
						auxSendValues.emplace_back(AkAuxSendValue{ g_listenerId, m_busId, amount });
						pObject->SetNeedsToUpdateEnvironments(true);
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
				auto const rtpcValue = static_cast<AkRtpcValue>(m_multiplier * amount + m_shift);

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
				AKRESULT const wwiseResult = AK::SoundEngine::SetRTPCValue(m_rtpcId, rtpcValue, pObject->GetId());

				if (!IS_WWISE_OK(wwiseResult))
				{
					Cry::Audio::Log(
						ELogType::Warning,
						"Wwise - failed to set the Rtpc %u to value %f on object %s in %s",
						m_rtpcId,
						rtpcValue,
						pObject->GetName()
						, __FUNCTION__);
				}
#else
				AK::SoundEngine::SetRTPCValue(m_rtpcId, rtpcValue, pObject->GetId());
#endif        // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
				break;
			}
		default:
			{
				CRY_ASSERT(false); //Unknown AudioEnvironmentImplementation type

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Wwise - Invalid EnvironmentData passed to the Wwise implementation of %s", __FUNCTION__);
	}
}
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
