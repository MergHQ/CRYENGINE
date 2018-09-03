// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObject.h"
#include "AudioEvent.h"
#include "ATLEntities.h"
#include "AudioImplCVars.h"
#include "Common.h"
#include <Logger.h>
#include <fmod_common.h>
#include <CryAudio/IAudioSystem.h>

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
extern TriggerToParameterIndexes g_triggerToParameterIndexes;

FMOD::Studio::System* CBaseObject::s_pSystem = nullptr;
FMOD::Studio::System* CListener::s_pSystem = nullptr;
FMOD::System* CStandaloneFileBase::s_pLowLevelSystem = nullptr;

static constexpr char const* s_szAbsoluteVelocityParameterName = "absolute_velocity";
static constexpr uint32 s_absoluteVelocityParameterId = StringToId(s_szAbsoluteVelocityParameterName);

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
				fmodResult = CStandaloneFileBase::s_pLowLevelSystem->createSound(soundInfo.name_or_data, mode, &soundInfo.exinfo, &pSound);
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
void CBaseObject::RemoveFile(CStandaloneFileBase const* const pFile)
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
			CStandaloneFileBase* pStandaloneFile = *iter;

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
	CStandaloneFileBase* const pStandaloneFile = static_cast<CStandaloneFileBase* const>(pIStandaloneFile);

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
	CStandaloneFileBase* const pStandaloneFile = static_cast<CStandaloneFileBase* const>(pIStandaloneFile);

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

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	CEnvironment const* const pEnvironment = static_cast<CEnvironment const* const>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		for (auto const pObject : m_objects)
		{
			if (pObject != this)
			{
				pObject->SetEnvironment(pEnvironment, amount);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid Environment pointer passed to the Fmod implementation of SetEnvironment");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	CParameter const* const pParameter = static_cast<CParameter const* const>(pIParameter);

	if (pParameter != nullptr)
	{
		for (auto const pObject : m_objects)
		{
			if (pObject != this)
			{
				pObject->SetParameter(pParameter, value);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid parameter pointer passed to the Fmod implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	CSwitchState const* const pSwitchState = static_cast<CSwitchState const* const>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		for (auto const pObject : m_objects)
		{
			if (pObject != this)
			{
				pObject->SetSwitchState(pISwitchState);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Fmod implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetOcclusionType(EOcclusionType const occlusionType)
{
	Cry::Audio::Log(ELogType::Error, "Trying to set occlusion type on the global object!");
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
	if (enable)
	{
		char const* szType = nullptr;

		switch (type)
		{
		case EObjectFunctionality::TrackAbsoluteVelocity:
			{
				szType = "absolute velocity tracking";

				break;
			}
		case EObjectFunctionality::TrackRelativeVelocity:
			{
				szType = "relative velocity tracking";

				break;
			}
		default:
			{
				szType = "an undefined functionality";

				break;
			}
		}

		Cry::Audio::Log(ELogType::Error, "Trying to enable %s on the global object!", szType);
	}
}

//////////////////////////////////////////////////////////////////////////
CObject::CObject(CObjectTransformation const& transformation)
	: m_flags(EObjectFlags::None)
	, m_previousAbsoluteVelocity(0.0f)
	, m_position(transformation.GetPosition())
	, m_previousPosition(transformation.GetPosition())
	, m_velocity(ZERO)
{
}

//////////////////////////////////////////////////////////////////////////
CObject::~CObject()
{
	if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
	{
		CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists.");
		g_numObjectsWithDoppler--;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::Update(float const deltaTime)
{
	if (((m_flags& EObjectFlags::MovingOrDecaying) != 0) && (deltaTime > 0.0f))
	{
		UpdateVelocities(deltaTime);
	}

	CBaseObject::Update(deltaTime);

#if !defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	// Always update in production code for debug draw.
	if ((m_flags& EObjectFlags::UpdateVirtualStates) != 0)
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
	{
		for (auto const pEvent : m_events)
		{
			pEvent->UpdateVirtualState();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetTransformation(CObjectTransformation const& transformation)
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
		Fill3DAttributeTransformation(m_transformation, m_attributes);

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			Fill3DAttributeVelocity(m_velocity, m_attributes);
		}

		Set3DAttributes();
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetEnvironment(IEnvironment const* const pIEnvironment, float const amount)
{
	auto const pEnvironment = static_cast<CEnvironment const*>(pIEnvironment);

	if (pEnvironment != nullptr)
	{
		bool bSet = true;
		auto const iter(m_environments.find(pEnvironment));

		if (iter != m_environments.end())
		{
			if (bSet = (fabs(iter->second - amount) > 0.001f))
			{
				iter->second = amount;
			}
		}
		else
		{
			m_environments.emplace(pEnvironment, amount);
		}

		if (bSet)
		{
			for (auto const pEvent : m_events)
			{
				pEvent->TrySetEnvironment(pEnvironment, amount);
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid IEnvironment pointer passed to the Fmod implementation of SetEnvironment");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameter(IParameter const* const pIParameter, float const value)
{
	auto const pParameter = static_cast<CParameter const*>(pIParameter);

	if (pParameter != nullptr)
	{
		EParameterType const type = pParameter->GetType();

		if (type == EParameterType::Parameter)
		{
			uint32 const parameterId = pParameter->GetId();
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

			for (auto const pEvent : m_events)
			{
				FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
				CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist.");
				CTrigger const* const pTrigger = pEvent->GetTrigger();
				CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist.");

				FMOD::Studio::EventDescription* pEventDescription = nullptr;
				fmodResult = pEventInstance->getDescription(&pEventDescription);
				ASSERT_FMOD_OK;

				if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
				{
					ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

					if (parameters.find(parameterId) != parameters.end())
					{
						fmodResult = pEventInstance->setParameterValueByIndex(parameters[parameterId], pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
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
								fmodResult = pEventInstance->setParameterValueByIndex(index, pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
								ASSERT_FMOD_OK;
								break;
							}
						}
					}
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
							g_triggerToParameterIndexes[pTrigger].emplace(std::make_pair(parameterId, index));
							fmodResult = pEventInstance->setParameterValueByIndex(index, pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
							ASSERT_FMOD_OK;
							break;
						}
					}
				}
			}

			auto const iter(m_parameters.find(pParameter));

			if (iter != m_parameters.end())
			{
				iter->second = value;
			}
			else
			{
				m_parameters.emplace(pParameter, value);
			}
		}
		else if (type == EParameterType::VCA)
		{
			auto const pVca = static_cast<CVcaParameter const* const>(pParameter);
			FMOD_RESULT const fmodResult = pVca->GetVca()->setVolume(pVca->GetValueMultiplier() * value + pVca->GetValueShift());
			ASSERT_FMOD_OK;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid AudioObjectData or ParameterData passed to the Fmod implementation of SetParameter");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetSwitchState(ISwitchState const* const pISwitchState)
{
	auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchState);

	if (pSwitchState != nullptr)
	{
		EStateType const type = pSwitchState->GetType();

		if (type == EStateType::State)
		{
			uint32 const parameterId = pSwitchState->GetId();
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

			for (auto const pEvent : m_events)
			{
				FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
				CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist.");
				CTrigger const* const pTrigger = pEvent->GetTrigger();
				CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist.");

				FMOD::Studio::EventDescription* pEventDescription = nullptr;
				fmodResult = pEventInstance->getDescription(&pEventDescription);
				ASSERT_FMOD_OK;

				if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
				{
					ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

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
							g_triggerToParameterIndexes[pTrigger].emplace(std::make_pair(parameterId, index));
							fmodResult = pEventInstance->setParameterValueByIndex(index, pSwitchState->GetValue());
							ASSERT_FMOD_OK;
							break;
						}
					}
				}
			}

			auto const iter(m_switches.find(pSwitchState->GetId()));

			if (iter != m_switches.end())
			{
				iter->second = pSwitchState;
			}
			else
			{
				m_switches.emplace(pSwitchState->GetId(), pSwitchState);
			}
		}
		else if (type == EStateType::VCA)
		{
			auto const pVca = static_cast<CVcaState const* const>(pSwitchState);
			FMOD_RESULT const fmodResult = pVca->GetVca()->setVolume(pVca->GetValue());
			ASSERT_FMOD_OK;
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Fmod implementation of SetSwitchState");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	for (auto const pEvent : m_events)
	{
		pEvent->SetOcclusion(occlusion);
	}

	m_occlusion = occlusion;
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetOcclusionType(EOcclusionType const occlusionType)
{
	if ((occlusionType != EOcclusionType::None) && (occlusionType != EOcclusionType::Ignore))
	{
		m_flags |= EObjectFlags::UpdateVirtualStates;
	}
	else
	{
		m_flags &= ~EObjectFlags::UpdateVirtualStates;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::ToggleFunctionality(EObjectFunctionality const type, bool const enable)
{
	switch (type)
	{
	case EObjectFunctionality::TrackAbsoluteVelocity:
		{
			if (enable)
			{
				m_flags |= EObjectFlags::TrackAbsoluteVelocity;
			}
			else
			{
				m_flags &= ~EObjectFlags::TrackAbsoluteVelocity;

				SetParameterById(s_absoluteVelocityParameterId, 0.0f);
			}

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			if (enable)
			{
				m_parameterInfo[s_szAbsoluteVelocityParameterName] = 0.0f;
			}
			else
			{
				m_parameterInfo.erase(s_szAbsoluteVelocityParameterName);
			}
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

			break;
		}
	case EObjectFunctionality::TrackRelativeVelocity:
		{
			if (enable)
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

					Vec3 const zeroVelocity{ 0.0f, 0.0f, 0.0f };
					Fill3DAttributeVelocity(zeroVelocity, m_attributes);
					Set3DAttributes();

					CRY_ASSERT_MESSAGE(g_numObjectsWithDoppler > 0, "g_numObjectsWithDoppler is 0 but an object with doppler tracking still exists.");
					g_numObjectsWithDoppler--;
				}
			}

			break;
		}
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY, char const* const szTextFilter)
{
#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)

	if (!m_parameterInfo.empty() || ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0))
	{
		bool isVirtual = true;

		for (auto const pEvent : m_events)
		{
			if (pEvent->GetState() != EEventState::Virtual)
			{
				isVirtual = false;
				break;
			}
		}

		for (auto const& parameterPair : m_parameterInfo)
		{
			bool canDraw = true;

			if (szTextFilter != nullptr)
			{
				CryFixedStringT<MaxControlNameLength> lowerCaseParameterName(parameterPair.first);
				lowerCaseParameterName.MakeLower();

				if (lowerCaseParameterName.find(szTextFilter) == CryFixedStringT<MaxControlNameLength>::npos)
				{
					canDraw = false;
				}
			}

			if (canDraw)
			{
				auxGeom.Draw2dLabel(
					posX,
					posY,
					g_debugObjectFontSize,
					isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
					false,
					"[Fmod] %s: %2.2f\n",
					parameterPair.first,
					parameterPair.second);

				posY += g_debugObjectLineHeight;
			}
		}

		if ((m_flags& EObjectFlags::TrackVelocityForDoppler) != 0)
		{
			auxGeom.Draw2dLabel(
				posX,
				posY,
				g_debugObjectFontSize,
				isVirtual ? g_debugObjectColorVirtual.data() : g_debugObjectColorPhysical.data(),
				false,
				"[Fmod] Doppler calculation enabled\n");
		}
	}

#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CObject::Set3DAttributes()
{
	for (auto const pEvent : m_events)
	{
		FMOD_RESULT const fmodResult = pEvent->GetInstance()->set3DAttributes(&m_attributes);
		ASSERT_FMOD_OK;
	}

	for (auto const pFile : m_files)
	{
		pFile->Set3DAttributes(m_attributes);
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
			Fill3DAttributeVelocity(m_velocity, m_attributes);
			Set3DAttributes();
		}
	}

	if ((m_flags& EObjectFlags::TrackAbsoluteVelocity) != 0)
	{
		float const absoluteVelocity = m_velocity.GetLength();

		if (absoluteVelocity == 0.0f || fabs(absoluteVelocity - m_previousAbsoluteVelocity) > g_cvars.m_velocityTrackingThreshold)
		{
			m_previousAbsoluteVelocity = absoluteVelocity;
			SetParameterById(s_absoluteVelocityParameterId, absoluteVelocity);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
			m_parameterInfo[s_szAbsoluteVelocityParameterName] = absoluteVelocity;
#endif      // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObject::SetParameterById(uint32 const parameterId, float const value)
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pEvent : m_events)
	{
		FMOD::Studio::EventInstance* const pEventInstance = pEvent->GetInstance();
		CRY_ASSERT_MESSAGE(pEventInstance != nullptr, "Event instance doesn't exist.");
		CTrigger const* const pTrigger = pEvent->GetTrigger();
		CRY_ASSERT_MESSAGE(pTrigger != nullptr, "Trigger doesn't exist.");

		FMOD::Studio::EventDescription* pEventDescription = nullptr;
		fmodResult = pEventInstance->getDescription(&pEventDescription);
		ASSERT_FMOD_OK;

		if (g_triggerToParameterIndexes.find(pTrigger) != g_triggerToParameterIndexes.end())
		{
			ParameterIdToIndex& parameters = g_triggerToParameterIndexes[pTrigger];

			if (parameters.find(parameterId) != parameters.end())
			{
				fmodResult = pEventInstance->setParameterValueByIndex(parameters[parameterId], value);
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
						fmodResult = pEventInstance->setParameterValueByIndex(index, value);
						ASSERT_FMOD_OK;
						break;
					}
				}
			}
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
					g_triggerToParameterIndexes[pTrigger].emplace(std::make_pair(parameterId, index));
					fmodResult = pEventInstance->setParameterValueByIndex(index, value);
					ASSERT_FMOD_OK;
					break;
				}
			}
		}
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
