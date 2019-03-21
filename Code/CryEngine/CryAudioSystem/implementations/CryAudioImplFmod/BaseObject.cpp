// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
#include "Impl.h"
#include "Event.h"
#include "EventInstance.h"
#include "Parameter.h"
#include "ParameterState.h"
#include "ParameterEnvironment.h"
#include "Return.h"

#include <CryAudio/IAudioSystem.h>

#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
	: m_flags(EObjectFlags::None)
	, m_occlusion(0.0f)
	, m_absoluteVelocity(0.0f)
{
	ZeroStruct(m_attributes);
	m_attributes.forward.z = 1.0f;
	m_attributes.up.y = 1.0f;

	m_eventInstances.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Update(float const deltaTime)
{
	EObjectFlags const previousFlags = m_flags;
	bool removedEvent = false;

	if (!m_eventInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	auto iter(m_eventInstances.begin());
	auto iterEnd(m_eventInstances.end());

	while (iter != iterEnd)
	{
		CEventInstance* const pEventInstance = *iter;

		if (pEventInstance->IsToBeRemoved())
		{
			ETriggerResult const result = (pEventInstance->GetState() == EEventState::Pending) ? ETriggerResult::Pending : ETriggerResult::Playing;
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEventInstance->GetTriggerInstanceId(), result);
			g_pImpl->DestructEventInstance(pEventInstance);
			removedEvent = true;

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_eventInstances.back();
			}

			m_eventInstances.pop_back();
			iter = m_eventInstances.begin();
			iterEnd = m_eventInstances.end();
		}
		else if ((pEventInstance->GetState() == EEventState::Pending))
		{
			if (SetEventInstance(pEventInstance))
			{
				ETriggerResult const result = (pEventInstance->GetState() == EEventState::Playing) ? ETriggerResult::Playing : ETriggerResult::Virtual;
				gEnv->pAudioSystem->ReportStartedTriggerConnectionInstance(pEventInstance->GetTriggerInstanceId(), result);

				UpdateVirtualFlag(pEventInstance);
			}

			++iter;
		}
		else
		{
			UpdateVirtualFlag(pEventInstance);

			++iter;
		}
	}

	if ((previousFlags != m_flags) && !m_eventInstances.empty())
	{
		if (((previousFlags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::IsVirtual) == 0))
		{
			gEnv->pAudioSystem->ReportPhysicalizedObject(this);
		}
		else if (((previousFlags& EObjectFlags::IsVirtual) == 0) && ((m_flags& EObjectFlags::IsVirtual) != 0))
		{
			gEnv->pAudioSystem->ReportVirtualizedObject(this);
		}
	}

	if (removedEvent)
	{
		UpdateVelocityTracking();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopAllTriggers()
{
	for (auto const pEventInstance : m_eventInstances)
	{
		pEventInstance->StopImmediate();
	}
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	m_name = szName;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::AddEventInstance(CEventInstance* const pEventInstance)
{
	m_eventInstances.push_back(pEventInstance);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEventInstance(uint32 const id)
{
	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->GetEvent().GetId() == id)
		{
			pEventInstance->StopAllowFadeOut();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetParameter(CParameterInfo& parameterInfo, float const value)
{
	if (parameterInfo.HasValidId())
	{
		for (auto const pEventInstance : m_eventInstances)
		{
			pEventInstance->GetFmodEventInstance()->setParameterByID(parameterInfo.GetId(), value);
		}

		m_parameters[parameterInfo] = value;
	}
	else
	{
		for (auto const pEventInstance : m_eventInstances)
		{
			FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;

			if (pEventInstance->GetEvent().GetEventDescription()->getParameterDescriptionByName(parameterInfo.GetName(), &parameterDescription) == FMOD_OK)
			{
				FMOD_STUDIO_PARAMETER_ID const id = parameterDescription.id;
				parameterInfo.SetId(id);

				for (auto const pEventInstance : m_eventInstances)
				{
					pEventInstance->GetFmodEventInstance()->setParameterByID(id, value);
				}

				m_parameters[parameterInfo] = value;

				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveParameter(CParameterInfo const& parameterInfo)
{
	m_parameters.erase(parameterInfo);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetReturn(CReturn const* const pReturn, float const amount)
{
	auto const iter(m_returns.find(pReturn));

	if (iter != m_returns.end())
	{
		if (fabs(iter->second - amount) > 0.001f)
		{
			iter->second = amount;

			for (auto const pEventInstance : m_eventInstances)
			{
				pEventInstance->SetReturnSend(pReturn, amount);
			}
		}
	}
	else
	{
		m_returns.emplace(pReturn, amount);

		for (auto const pEventInstance : m_eventInstances)
		{
			pEventInstance->SetReturnSend(pReturn, amount);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveReturn(CReturn const* const pReturn)
{
	m_returns.erase(pReturn);
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetEventInstance(CEventInstance* const pEventInstance)
{
	bool bSuccess = false;

	// Update the event with all parameter and environment values
	// that are currently set on the object before starting it.
	if (pEventInstance->PrepareForOcclusion())
	{
		FMOD::Studio::EventInstance* const pFModEventInstance = pEventInstance->GetFmodEventInstance();
		CRY_ASSERT_MESSAGE(pFModEventInstance != nullptr, "Fmod event instance doesn't exist during %s", __FUNCTION__);

		for (auto const& parameterPair : m_parameters)
		{
			pFModEventInstance->setParameterByID(parameterPair.first.GetId(), parameterPair.second);
		}

		for (auto const& returnPair : m_returns)
		{
			pEventInstance->SetReturnSend(returnPair.first, returnPair.second);
		}

		UpdateVelocityTracking();
		pEventInstance->SetOcclusion(m_occlusion);

		FMOD_RESULT const fmodResult = pFModEventInstance->start();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		pEventInstance->UpdateVirtualState();
		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVirtualFlag(CEventInstance* const pEventInstance)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE)
	// Always update in production code for debug draw.
	pEventInstance->UpdateVirtualState();

	if (pEventInstance->GetState() != EEventState::Virtual)
	{
		m_flags &= ~EObjectFlags::IsVirtual;
	}
#else
	if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
	{
		pEventInstance->UpdateVirtualState();

		if (pEventInstance->GetState() != EEventState::Virtual)
		{
			m_flags &= ~EObjectFlags::IsVirtual;
		}
	}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_DEBUG_CODE
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVelocityTracking()
{
	bool trackVelocity = false;

	for (auto const pEventInstance : m_eventInstances)
	{
		if ((pEventInstance->GetEvent().GetFlags() & EEventFlags::HasAbsoluteVelocityParameter) != 0)
		{
			trackVelocity = true;
			break;
		}
	}

	trackVelocity ? (m_flags |= EObjectFlags::TrackAbsoluteVelocity) : (m_flags &= ~EObjectFlags::TrackAbsoluteVelocity);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
