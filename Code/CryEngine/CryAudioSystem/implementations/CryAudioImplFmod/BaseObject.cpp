// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
#include "Impl.h"
#include "BaseStandaloneFile.h"
#include "Event.h"
#include "Environment.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Trigger.h"

#include <Logger.h>
#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
	: m_flags(EObjectFlags::None)
{
	ZeroStruct(m_attributes);
	m_attributes.forward.z = 1.0f;
	m_attributes.up.y = 1.0f;

	// Reserve enough room for events to minimize/prevent runtime allocations.
	m_events.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveFile(CBaseStandaloneFile const* const pFile)
{
	if (!stl::find_and_erase(m_files, pFile))
	{
		if (!stl::find_and_erase(m_pendingFiles, pFile))
		{
			Cry::Audio::Log(ELogType::Error, "Tried to remove an audio file from an object that is not playing that file");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetEvent(CEvent* const pEvent)
{
	bool bSuccess = false;

	// Update the event with all parameter and switch values
	// that are currently set on the object before starting it.
	if (pEvent->PrepareForOcclusion())
	{
		m_events.push_back(pEvent);

		FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
		CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist during %s", __FUNCTION__);
		CTrigger const* const pTrigger = pEvent->GetTrigger();
		CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist during %s", __FUNCTION__);

		FMOD::Studio::EventDescription* pEventDescription = nullptr;
		FMOD_RESULT fmodResult = pEventInstance->getDescription(&pEventDescription);
		ASSERT_FMOD_OK;

		if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
		{
			ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

			for (auto const& parameterPair : m_parameters)
			{
				uint32 const parameterId = parameterPair.first->GetId();

				if (parameters.find(parameterId) != parameters.end())
				{
					fmodResult = pEventInstance->setParameterValueByIndex(parameters[parameterId], parameterPair.second);
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
							fmodResult = pEventInstance->setParameterValueByIndex(index, parameterPair.second);
							ASSERT_FMOD_OK;
							break;
						}
					}
				}
			}

			for (auto const& switchPair : m_switches)
			{
				CBaseSwitchState const* const pSwitchState = switchPair.second;
				uint32 const parameterId = pSwitchState->GetId();

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
		}

		for (auto const& environmentPair : m_environments)
		{
			pEvent->TrySetEnvironment(environmentPair.first, environmentPair.second);
		}

		UpdateVelocityTracking();
		pEvent->SetOcclusion(m_occlusion);
		pEvent->SetAbsoluteVelocity(m_absoluteVelocity);

		fmodResult = pEvent->GetInstance()->start();
		ASSERT_FMOD_OK;

		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVelocityTracking()
{
	bool trackVelocity = false;

	for (auto const pEvent : m_events)
	{
		if (pEvent->HasAbsoluteVelocityParameter())
		{
			trackVelocity = true;
			break;
		}
	}

	trackVelocity ? (m_flags |= EObjectFlags::TrackAbsoluteVelocity) : (m_flags &= ~EObjectFlags::TrackAbsoluteVelocity);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveParameter(CBaseParameter const* const pParameter)
{
	m_parameters.erase(pParameter);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveSwitch(CBaseSwitchState const* const pSwitch)
{
	m_switches.erase(pSwitch->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveEnvironment(CEnvironment const* const pEnvironment)
{
	m_environments.erase(pEnvironment);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Update(float const deltaTime)
{
	if (!m_pendingEvents.empty())
	{
		auto iter(m_pendingEvents.begin());
		auto iterEnd(m_pendingEvents.cend());

		while (iter != iterEnd)
		{
			if (SetEvent(*iter))
			{
				iter = m_pendingEvents.erase(iter);
				iterEnd = m_pendingEvents.cend();
				continue;
			}

			++iter;
		}
	}

	if (!m_pendingFiles.empty())
	{
		auto iter(m_pendingFiles.begin());
		auto iterEnd(m_pendingFiles.cend());

		while (iter != iterEnd)
		{
			CBaseStandaloneFile* pStandaloneFile = *iter;

			if (pStandaloneFile->IsReady())
			{
				m_files.push_back(pStandaloneFile);

				pStandaloneFile->PlayFile(m_attributes);

				iter = m_pendingFiles.erase(iter);
				iterEnd = m_pendingFiles.cend();
				continue;
			}
			++iter;
		}
	}

	EObjectFlags const previousFlags = m_flags;
	bool removedEvent = false;

	if (!m_events.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	auto iter(m_events.begin());
	auto iterEnd(m_events.end());

	while (iter != iterEnd)
	{
		auto const pEvent = *iter;

		if (pEvent->IsToBeRemoved())
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEvent->GetTriggerInstanceId());
			g_pImpl->DestructEvent(pEvent);
			removedEvent = true;

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_events.back();
			}

			m_events.pop_back();
			iter = m_events.begin();
			iterEnd = m_events.end();
		}
		else
		{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			// Always update in production code for debug draw.
			pEvent->UpdateVirtualState();

			if (pEvent->GetState() != EEventState::Virtual)
			{
				m_flags &= ~EObjectFlags::IsVirtual;
			}
#else
			if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
			{
				pEvent->UpdateVirtualState();

				if (pEvent->GetState() != EEventState::Virtual)
				{
					m_flags &= ~EObjectFlags::IsVirtual;
				}
			}
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
			++iter;
		}
	}

	if ((previousFlags != m_flags) && (!m_events.empty() || !m_pendingEvents.empty()))
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
	for (auto const pEvent : m_events)
	{
		pEvent->StopImmediate();
	}
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEvent(uint32 const id)
{
	for (auto const pEvent : m_events)
	{
		if (pEvent->GetId() == id)
		{
			pEvent->StopAllowFadeOut();
		}
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
