// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
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
CBaseObject::~CBaseObject()
{
	// If the object is deleted before its events get
	// cleared we need to remove all references to it
	for (auto const pEvent : m_events)
	{
		pEvent->SetObject(nullptr);
	}

	for (auto const pEvent : m_pendingEvents)
	{
		pEvent->SetObject(nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveEvent(CEvent* const pEvent)
{
	if (!stl::find_and_erase(m_events, pEvent))
	{
		if (!stl::find_and_erase(m_pendingEvents, pEvent))
		{
			Cry::Audio::Log(ELogType::Error, "Tried to remove an event from an object that does not own that event");
		}
	}
	else
	{
		UpdateVelocityTracking();
	}
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

		pEvent->UpdateVirtualState();

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
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopAllTriggers()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pEvent : m_events)
	{
		fmodResult = pEvent->GetInstance()->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		ASSERT_FMOD_OK;
	}
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
	// Fmod does not have the concept of objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEvent(uint32 const id)
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pEvent : m_events)
	{
		if (pEvent->GetId() == id)
		{
			fmodResult = pEvent->GetInstance()->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
			ASSERT_FMOD_OK;
		}
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
