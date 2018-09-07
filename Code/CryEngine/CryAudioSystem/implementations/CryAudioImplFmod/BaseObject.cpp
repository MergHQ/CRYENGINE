// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
#include "BaseStandaloneFile.h"
#include "Event.h"
#include "Environment.h"
#include "Listener.h"
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
FMOD::Studio::System* CBaseObject::s_pSystem = nullptr;
FMOD::Studio::System* CListener::s_pSystem = nullptr;
FMOD::System* CBaseStandaloneFile::s_pLowLevelSystem = nullptr;

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK EventCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* event, void* parameters)
{
	FMOD::Studio::EventInstance* const pEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(event);

	if (pEventInstance != nullptr)
	{
		CEvent* pEvent = nullptr;
		FMOD_RESULT const fmodResult = pEventInstance->getUserData(reinterpret_cast<void**>(&pEvent));
		ASSERT_FMOD_OK;

		if (pEvent != nullptr)
		{
			gEnv->pAudioSystem->ReportFinishedEvent(pEvent->GetATLEvent(), true);
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK ProgrammerSoundCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* pEventInst, void* pInOutParameters)
{
	if (pEventInst != nullptr)
	{
		auto const pEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(pEventInst);
		CEvent* pEvent = nullptr;
		FMOD_RESULT fmodResult = pEventInstance->getUserData(reinterpret_cast<void**>(&pEvent));
		ASSERT_FMOD_OK;

		if ((pEvent != nullptr) && (pEvent->GetTrigger() != nullptr))
		{
			if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
			{
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer");
				auto const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);
				char const* const szKey = pEvent->GetTrigger()->GetKey().c_str();

				FMOD_STUDIO_SOUND_INFO soundInfo;
				fmodResult = CBaseObject::s_pSystem->getSoundInfo(szKey, &soundInfo);
				ASSERT_FMOD_OK;

				FMOD::Sound* pSound = nullptr;
				FMOD_MODE const mode = FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING | FMOD_3D | soundInfo.mode;
				fmodResult = CBaseStandaloneFile::s_pLowLevelSystem->createSound(soundInfo.name_or_data, mode, &soundInfo.exinfo, &pSound);
				ASSERT_FMOD_OK;

				pInOutProperties->sound = reinterpret_cast<FMOD_SOUND*>(pSound);
				pInOutProperties->subsoundIndex = soundInfo.subsoundindex;
			}
			else if (type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
			{
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer");
				auto const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				auto* pSound = reinterpret_cast<FMOD::Sound*>(pInOutProperties->sound);

				fmodResult = pSound->release();
				ASSERT_FMOD_OK;
			}
			else if ((type == FMOD_STUDIO_EVENT_CALLBACK_START_FAILED) || (type == FMOD_STUDIO_EVENT_CALLBACK_STOPPED))
			{
				ASSERT_FMOD_OK;
				gEnv->pAudioSystem->ReportFinishedEvent(pEvent->GetATLEvent(), true);
			}
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
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
		CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist.");
		CTrigger const* const pTrigger = pEvent->GetTrigger();
		CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist.");

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
				CSwitchState const* const pSwitchState = switchPair.second;
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

		pEvent->SetOcclusion(m_occlusion);

		fmodResult = pEvent->GetInstance()->start();
		ASSERT_FMOD_OK;

		pEvent->UpdateVirtualState();

		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveParameter(CParameter const* const pParameter)
{
	m_parameters.erase(pParameter);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveSwitch(CSwitchState const* const pSwitch)
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

				pStandaloneFile->Play(m_attributes);

				iter = m_pendingFiles.erase(iter);
				iterEnd = m_pendingFiles.cend();
				continue;
			}
			++iter;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::ExecuteTrigger(ITrigger const* const pITrigger, IEvent* const pIEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;
	auto const pTrigger = static_cast<CTrigger const*>(pITrigger);
	auto const pEvent = static_cast<CEvent*>(pIEvent);

	if ((pTrigger != nullptr) && (pEvent != nullptr))
	{
		pEvent->SetTrigger(pTrigger);
		auto const type = pTrigger->GetEventType();

		switch (type)
		{
		case EEventType::Start:
			{
				FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
				FMOD::Studio::EventDescription* pEventDescription = pTrigger->GetEventDescription();

				if (pEventDescription == nullptr)
				{
					FMOD_GUID const guid = pTrigger->GetGuid();
					fmodResult = s_pSystem->getEventByID(&guid, &pEventDescription);
					ASSERT_FMOD_OK;
				}

				if (pEventDescription != nullptr)
				{
					CRY_ASSERT(pEvent->GetInstance() == nullptr);

					FMOD::Studio::EventInstance* pInstance = nullptr;
					fmodResult = pEventDescription->createInstance(&pInstance);
					ASSERT_FMOD_OK;
					pEvent->SetInstance(pInstance);

					if (pTrigger->HasProgrammerSound())
					{
						fmodResult = pEvent->GetInstance()->setCallback(ProgrammerSoundCallback);
					}
					else
					{
						fmodResult = pEvent->GetInstance()->setCallback(EventCallback, FMOD_STUDIO_EVENT_CALLBACK_START_FAILED | FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
					}

					ASSERT_FMOD_OK;
					fmodResult = pEvent->GetInstance()->setUserData(pEvent);
					ASSERT_FMOD_OK;
					fmodResult = pEvent->GetInstance()->set3DAttributes(&m_attributes);
					ASSERT_FMOD_OK;

					CRY_ASSERT(pEvent->GetId() == InvalidCRC32);
					pEvent->SetId(pTrigger->GetId());
					pEvent->SetObject(this);

					CRY_ASSERT_MESSAGE(std::find(m_pendingEvents.begin(), m_pendingEvents.end(), pEvent) == m_pendingEvents.end(), "Event was already in the pending list");
					m_pendingEvents.push_back(pEvent);
					requestResult = ERequestStatus::Success;
				}
			}
			break;
		case EEventType::Stop:
			{
				StopEvent(pTrigger->GetId());
				requestResult = ERequestStatus::SuccessDoNotTrack;
			}
			break;
		case EEventType::Pause:
		case EEventType::Resume:
			{
				FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
				FMOD::Studio::EventDescription* pEventDescription = pTrigger->GetEventDescription();

				if (pEventDescription == nullptr)
				{
					FMOD_GUID const guid = pTrigger->GetGuid();
					fmodResult = s_pSystem->getEventByID(&guid, &pEventDescription);
					ASSERT_FMOD_OK;
				}

				if (pEventDescription != nullptr)
				{
					CRY_ASSERT(pEvent->GetInstance() == nullptr);

					bool const shouldPause = (type == EEventType::Pause);
					int const capacity = 32;
					int count = 0;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
					fmodResult = pEventDescription->getInstanceCount(&count);
					ASSERT_FMOD_OK;
					CRY_ASSERT_MESSAGE(count < capacity, "Instance count (%d) is higher or equal than array capacity (%d).", count, capacity);
#endif          // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

					FMOD::Studio::EventInstance* eventInstances[capacity];
					fmodResult = pEventDescription->getInstanceList(eventInstances, capacity, &count);
					ASSERT_FMOD_OK;

					for (int i = 0; i < count; ++i)
					{
						auto const pInstance = eventInstances[i];

						if (pInstance != nullptr)
						{
							fmodResult = pInstance->setPaused(shouldPause);
							ASSERT_FMOD_OK;
						}
					}

					requestResult = ERequestStatus::SuccessDoNotTrack;
				}
			}
			break;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid trigger or event pointers passed to the Fmod implementation of ExecuteTrigger.");
	}

	return requestResult;
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
ERequestStatus CBaseObject::PlayFile(IStandaloneFile* const pIStandaloneFile)
{
	CBaseStandaloneFile* const pStandaloneFile = static_cast<CBaseStandaloneFile* const>(pIStandaloneFile);

	if (pStandaloneFile != nullptr)
	{
		pStandaloneFile->m_pObject = this;
		pStandaloneFile->StartLoading();

		CRY_ASSERT_MESSAGE(std::find(m_pendingFiles.begin(), m_pendingFiles.end(), pStandaloneFile) == m_pendingFiles.end(), "standalone file was already in the pending standalone files list");
		m_pendingFiles.push_back(pStandaloneFile);
		return ERequestStatus::Success;
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid standalone file pointer passed to the Fmod implementation of PlayFile.");
	}

	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::StopFile(IStandaloneFile* const pIStandaloneFile)
{
	CBaseStandaloneFile* const pStandaloneFile = static_cast<CBaseStandaloneFile* const>(pIStandaloneFile);

	if (pStandaloneFile != nullptr)
	{
		pStandaloneFile->Stop();
		return ERequestStatus::Pending;
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid standalone file pointer passed to the Fmod implementation of StopFile.");
	}

	return ERequestStatus::Failure;
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
