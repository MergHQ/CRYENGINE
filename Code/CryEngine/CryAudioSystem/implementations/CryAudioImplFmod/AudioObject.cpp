// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObject.h"
#include "AudioEvent.h"
#include "ATLEntities.h"
#include "SharedAudioData.h"
#include <AudioLogger.h>
#include <fmod_common.h>

using namespace CryAudio;
using namespace CryAudio::Impl;
using namespace CryAudio::Impl::Fmod;

extern FmodSwitchToIndexMap g_switchToIndex;
extern AudioParameterToIndexMap g_parameterToIndex;

FMOD::Studio::System* CAudioObjectBase::s_pSystem = nullptr;
FMOD::Studio::System* CAudioListener::s_pSystem = nullptr;
FMOD::System* CAudioFileBase::s_pLowLevelSystem = nullptr;

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK EventCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* event, void* parameters)
{
	FMOD::Studio::EventInstance* const pEvent = reinterpret_cast<FMOD::Studio::EventInstance*>(event);

	if (pEvent != nullptr)
	{
		CAudioEvent* pAudioEvent = nullptr;
		FMOD_RESULT const fmodResult = pEvent->getUserData(reinterpret_cast<void**>(&pAudioEvent));
		ASSERT_FMOD_OK;

		if (pAudioEvent != nullptr)
		{
			gEnv->pAudioSystem->ReportFinishedEvent(pAudioEvent->GetATLEvent(), true);
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
CAudioObjectBase::CAudioObjectBase()
{
	ZeroStruct(m_attributes);

	// Reserve enough room for events to minimize/prevent runtime allocations.
	m_audioEvents.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
CAudioObjectBase::~CAudioObjectBase()
{
	// If the audio object is deleted before its events get
	// cleared we need to remove all references to it
	for (auto const pAudioEvent : m_audioEvents)
	{
		pAudioEvent->SetAudioObject(nullptr);
	}

	for (auto const pAudioEvent : m_pendingAudioEvents)
	{
		pAudioEvent->SetAudioObject(nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::RemoveEvent(CAudioEvent* const pAudioEvent)
{
	if (!stl::find_and_erase(m_audioEvents, pAudioEvent))
	{
		if (!stl::find_and_erase(m_pendingAudioEvents, pAudioEvent))
		{
			CRY_ASSERT_MESSAGE(true, "Tried to remove an event from an audio object that does not own that event");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::RemoveFile(CAudioFileBase const* const pFile)
{
	if (!stl::find_and_erase(m_files, pFile))
	{
		if (!stl::find_and_erase(m_pendingFiles, pFile))
		{
			CRY_ASSERT_MESSAGE(true, "Tried to remove an audio file from an audio object that is not playing that file");
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectBase::SetAudioEvent(CAudioEvent* const pAudioEvent)
{
	bool bSuccess = false;

	// Update the event with all parameter and switch values
	// that are currently set on the audio object before starting it.
	if (pAudioEvent->PrepareForOcclusion())
	{
		m_audioEvents.push_back(pAudioEvent);
		FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

		for (auto const& parameterPair : m_audioParameters)
		{
			AudioParameterToIndexMap::iterator const iter(g_parameterToIndex.find(parameterPair.first));

			if (iter != g_parameterToIndex.end())
			{
				if (pAudioEvent->GetEventPathId() == iter->first->GetEventPathId())
				{
					if (iter->second != FMOD_IMPL_INVALID_INDEX)
					{
						FMOD::Studio::ParameterInstance* pParameterInstance = nullptr;
						pAudioEvent->GetInstance()->getParameterByIndex(iter->second, &pParameterInstance);

						if (pParameterInstance != nullptr)
						{
							fmodResult = pParameterInstance->setValue(parameterPair.second);
							ASSERT_FMOD_OK;
						}
					}
					else
					{
						SetParameter(iter->first, parameterPair.second);
					}
				}
			}
			else
			{
				g_implLogger.Log(ELogType::Warning, "Trying to set an unknown Fmod parameter during \"AddAudioEvent\": %s", parameterPair.first->GetName().c_str());
			}
		}

		for (auto const& switchPair : m_audioSwitches)
		{
			FmodSwitchToIndexMap::iterator const iter(g_switchToIndex.find(switchPair.second));

			if (iter != g_switchToIndex.end())
			{
				if (pAudioEvent->GetEventPathId() == iter->first->eventPathId)
				{
					if (iter->second != FMOD_IMPL_INVALID_INDEX)
					{
						FMOD::Studio::ParameterInstance* pParameterInstance = nullptr;
						pAudioEvent->GetInstance()->getParameterByIndex(iter->second, &pParameterInstance);

						if (pParameterInstance != nullptr)
						{
							fmodResult = pParameterInstance->setValue(switchPair.second->value);
							ASSERT_FMOD_OK;
						}
					}
					else
					{
						SetSwitchState(switchPair.second);
					}
				}
			}
			else
			{
				g_implLogger.Log(ELogType::Warning, "Trying to set an unknown Fmod switch during \"AddAudioEvent\": %s", switchPair.second->name.c_str());
			}
		}

		for (auto const& environmentPair : m_audioEnvironments)
		{
			pAudioEvent->TrySetEnvironment(environmentPair.first, environmentPair.second);
		}

		pAudioEvent->SetObstructionOcclusion(m_obstruction, m_occlusion);

		fmodResult = pAudioEvent->GetInstance()->start();
		ASSERT_FMOD_OK;

		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::RemoveParameter(CAudioParameter const* const pParameter)
{
	m_audioParameters.erase(pParameter);
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::RemoveSwitch(CAudioSwitchState const* const pSwitch)
{
	m_audioSwitches.erase(pSwitch->eventPathId);
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::RemoveEnvironment(CAudioEnvironment const* const pEnvironment)
{
	m_audioEnvironments.erase(pEnvironment);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::Update()
{
	if (!m_pendingAudioEvents.empty())
	{
		auto iter(m_pendingAudioEvents.begin());
		auto iterEnd(m_pendingAudioEvents.cend());

		while (iter != iterEnd)
		{
			if (SetAudioEvent(*iter))
			{
				iter = m_pendingAudioEvents.erase(iter);
				iterEnd = m_pendingAudioEvents.cend();
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
			CAudioFileBase* pFile = *iter;

			if (pFile->IsReady())
			{
				m_files.push_back(pFile);

				pFile->Play(m_attributes);

				iter = m_pendingFiles.erase(iter);
				iterEnd = m_pendingFiles.cend();
				continue;
			}
			++iter;
		}
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::Set3DAttributes(SObject3DAttributes const& attributes)
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
	FillFmodObjectPosition(attributes, m_attributes);

	for (auto const pAudioEvent : m_audioEvents)
	{
		fmodResult = pAudioEvent->GetInstance()->set3DAttributes(&m_attributes);
		ASSERT_FMOD_OK;
	}

	for (auto const pFile : m_files)
	{
		pFile->Set3DAttributes(m_attributes);
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::ExecuteTrigger(IAudioTrigger const* const pIAudioTrigger, IAudioEvent* const pIAudioEvent)
{
	ERequestStatus requestResult = ERequestStatus::Failure;
	CAudioTrigger const* const pAudioTrigger = static_cast<CAudioTrigger const* const>(pIAudioTrigger);
	CAudioEvent* const pAudioEvent = static_cast<CAudioEvent*>(pIAudioEvent);

	if ((pAudioTrigger != nullptr) && (pAudioEvent != nullptr))
	{
		if (pAudioTrigger->m_eventType == EFmodEventType::Start)
		{
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
			FMOD::Studio::EventDescription* pEventDescription = pAudioTrigger->m_pEventDescription;

			if (pEventDescription == nullptr)
			{
				fmodResult = s_pSystem->getEventByID(&pAudioTrigger->m_guid, &pEventDescription);
				ASSERT_FMOD_OK;
			}

			if (pEventDescription != nullptr)
			{
				CRY_ASSERT(pAudioEvent->GetInstance() == nullptr);

				FMOD::Studio::EventInstance* pInstance = nullptr;
				fmodResult = pEventDescription->createInstance(&pInstance);
				ASSERT_FMOD_OK;
				pAudioEvent->SetInstance(pInstance);
				fmodResult = pAudioEvent->GetInstance()->setCallback(EventCallback, FMOD_STUDIO_EVENT_CALLBACK_START_FAILED | FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
				ASSERT_FMOD_OK;
				fmodResult = pAudioEvent->GetInstance()->setUserData(pAudioEvent);
				ASSERT_FMOD_OK;
				fmodResult = pAudioEvent->GetInstance()->set3DAttributes(&m_attributes);
				ASSERT_FMOD_OK;

				CRY_ASSERT(pAudioEvent->GetEventPathId() == AUDIO_INVALID_CRC32);
				pAudioEvent->SetEventPathId(pAudioTrigger->m_eventPathId);
				pAudioEvent->SetAudioObject(this);

				CRY_ASSERT_MESSAGE(std::find(m_pendingAudioEvents.begin(), m_pendingAudioEvents.end(), pAudioEvent) == m_pendingAudioEvents.end(), "Event was already in the pending list");
				m_pendingAudioEvents.push_back(pAudioEvent);
				requestResult = ERequestStatus::Success;
			}
		}
		else
		{
			StopEvent(pAudioTrigger->m_eventPathId);

			// Return failure here so the ATL does not keep track of this event.
			requestResult = ERequestStatus::Failure;
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData, ATLTriggerData or EventData passed to the Fmod implementation of ExecuteTrigger.");
	}

	return requestResult;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::StopAllTriggers()
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pAudioEvent : m_audioEvents)
	{
		fmodResult = pAudioEvent->GetInstance()->stop(FMOD_STUDIO_STOP_IMMEDIATE);
		ASSERT_FMOD_OK;
	}

	return ERequestStatus::Success;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::PlayFile(IAudioStandaloneFile* const pIFile)
{
	CAudioFileBase* const pFile = static_cast<CAudioFileBase* const>(pIFile);
	if (pFile != nullptr)
	{
		pFile->m_pAudioObject = this;
		pFile->StartLoading();

		CRY_ASSERT_MESSAGE(std::find(m_pendingFiles.begin(), m_pendingFiles.end(), pFile) == m_pendingFiles.end(), "standalone file was already in the pending standalone files list");
		m_pendingFiles.push_back(pFile);

		return ERequestStatus::Success;
	}

	g_implLogger.Log(ELogType::Error, "Invalid AudioObject, AudioTrigger or StandaloneFile passed to the Fmod implementation of PlayFile.");
	return ERequestStatus::Failure;

}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::StopFile(IAudioStandaloneFile* const pIFile)
{
	CAudioFileBase* const pFile = static_cast<CAudioFileBase* const>(pIFile);

	if (pFile != nullptr)
	{
		pFile->Stop();
		return ERequestStatus::Pending;
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid SAudioStandaloneFileInfo passed to the Fmod implementation of StopFile.");
	}

	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObjectBase::SetName(char const* const szName)
{
	// Fmod does not have the concept of audio objects and with that the debugging of such.
	// Therefore the name is currently not needed here.
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectBase::StopEvent(uint32 const eventPathId)
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pAudioEvent : m_audioEvents)
	{
		if (pAudioEvent->GetEventPathId() == eventPathId)
		{
			fmodResult = pAudioEvent->GetInstance()->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
			ASSERT_FMOD_OK;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CGlobalAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	ERequestStatus result = ERequestStatus::Failure;
	CAudioEnvironment const* const pEnvironment = static_cast<CAudioEnvironment const* const>(pIAudioEnvironment);

	if (pEnvironment != nullptr)
	{
		for (auto const pAudioObject : m_audioObjectsList)
		{
			if (pAudioObject != this)
			{
				pAudioObject->SetEnvironment(pEnvironment, amount);
			}
		}
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid Environment pointer passed to the Fmod implementation of SetEnvironment");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CGlobalAudioObject::SetParameter(IParameter const* const pIAudioParameter, float const value)
{
	ERequestStatus result = ERequestStatus::Failure;
	CAudioParameter const* const pParameter = static_cast<CAudioParameter const* const>(pIAudioParameter);

	if (pParameter != nullptr)
	{
		for (auto const pAudioObject : m_audioObjectsList)
		{
			if (pAudioObject != this)
			{
				pAudioObject->SetParameter(pParameter, value);
			}
		}
		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData or RtpcData passed to the Fmod implementation of SetRtpc");
	}

	return result;
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CGlobalAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	ERequestStatus result = ERequestStatus::Failure;
	CAudioSwitchState const* const pSwitchState = static_cast<CAudioSwitchState const* const>(pIAudioSwitchState);

	if (pSwitchState != nullptr)
	{
		for (auto const pAudioObject : m_audioObjectsList)
		{
			if (pAudioObject != this)
			{
				pAudioObject->SetSwitchState(pIAudioSwitchState);
			}
		}

		result = ERequestStatus::Success;
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData or RtpcData passed to the Fmod implementation of SetSwitchState");
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CGlobalAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	g_implLogger.Log(ELogType::Error, "Trying to set occlusion and obstruction values on the global audio object!");
	return ERequestStatus::Failure;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetEnvironment(IAudioEnvironment const* const pIAudioEnvironment, float const amount)
{
	ERequestStatus result = ERequestStatus::Success;
	CAudioEnvironment const* const pEnvironment = static_cast<CAudioEnvironment const* const>(pIAudioEnvironment);
	if (pEnvironment != nullptr)
	{
		bool bSet = true;
		auto const iter(m_audioEnvironments.find(pEnvironment));

		if (iter != m_audioEnvironments.end())
		{
			if (bSet = (fabs(iter->second - amount) > 0.001f))
			{
				iter->second = amount;
			}
		}
		else
		{
			m_audioEnvironments.emplace(pEnvironment, amount);
		}

		if (bSet)
		{
			for (auto const pAudioEvent : m_audioEvents)
			{
				pAudioEvent->TrySetEnvironment(pEnvironment, amount);
			}
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid Environment pointer passed to the Fmod implementation of SetEnvironment");
		result = ERequestStatus::Failure;

	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetParameter(IParameter const* const pIAudioParameter, float const value)
{
	ERequestStatus result = ERequestStatus::Success;
	CAudioParameter const* const pParameter = static_cast<CAudioParameter const* const>(pIAudioParameter);

	if (pParameter != nullptr)
	{
		FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

		for (auto const pAudioEvent : m_audioEvents)
		{
			if (pAudioEvent->GetEventPathId() == pParameter->GetEventPathId())
			{
				FMOD::Studio::ParameterInstance* pParameterInstance = nullptr;
				AudioParameterToIndexMap::iterator const iter(g_parameterToIndex.find(pParameter));

				if (iter != g_parameterToIndex.end())
				{
					if (iter->second != FMOD_IMPL_INVALID_INDEX)
					{
						pAudioEvent->GetInstance()->getParameterByIndex(iter->second, &pParameterInstance);

						if (pParameterInstance != nullptr)
						{
							fmodResult = pParameterInstance->setValue(pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
							ASSERT_FMOD_OK;
						}
						else
						{
							g_implLogger.Log(ELogType::Warning, "Unknown Fmod parameter index (%d) for (%s)", iter->second, pParameter->GetName().c_str());
						}
					}
					else
					{
						int parameterCount = 0;
						fmodResult = pAudioEvent->GetInstance()->getParameterCount(&parameterCount);
						ASSERT_FMOD_OK;

						for (int index = 0; index < parameterCount; ++index)
						{
							fmodResult = pAudioEvent->GetInstance()->getParameterByIndex(index, &pParameterInstance);
							ASSERT_FMOD_OK;

							FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
							fmodResult = pParameterInstance->getDescription(&parameterDescription);
							ASSERT_FMOD_OK;

							if (pParameter->GetName().compareNoCase(parameterDescription.name) == 0)
							{
								iter->second = index;
								fmodResult = pParameterInstance->setValue(pParameter->GetValueMultiplier() * value + pParameter->GetValueShift());
								ASSERT_FMOD_OK;

								break;
							}
						}
					}
				}
				else
				{
					g_implLogger.Log(ELogType::Warning, "Trying to set an unknown Fmod parameter: %s", pParameter->GetName().c_str());
				}
			}
		}

		auto const iter(m_audioParameters.find(pParameter));

		if (iter != m_audioParameters.end())
		{
			iter->second = value;
		}
		else
		{
			m_audioParameters.emplace(pParameter, value);
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData or RtpcData passed to the Fmod implementation of SetRtpc");
		result = ERequestStatus::Failure;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetSwitchState(IAudioSwitchState const* const pIAudioSwitchState)
{
	ERequestStatus result = ERequestStatus::Success;
	CAudioSwitchState const* const pSwitchState = static_cast<CAudioSwitchState const* const>(pIAudioSwitchState);

	if (pSwitchState != nullptr)
	{
		FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;
		for (auto const pAudioEvent : m_audioEvents)
		{
			if (pAudioEvent->GetEventPathId() == pSwitchState->eventPathId)
			{
				FMOD::Studio::ParameterInstance* pParameterInstance = nullptr;
				FmodSwitchToIndexMap::iterator const iter(g_switchToIndex.find(pSwitchState));

				if (iter != g_switchToIndex.end())
				{
					if (iter->second != FMOD_IMPL_INVALID_INDEX)
					{
						pAudioEvent->GetInstance()->getParameterByIndex(iter->second, &pParameterInstance);

						if (pParameterInstance != nullptr)
						{
							fmodResult = pParameterInstance->setValue(pSwitchState->value);
							ASSERT_FMOD_OK;
						}
					}
					else
					{
						int parameterCount = 0;
						fmodResult = pAudioEvent->GetInstance()->getParameterCount(&parameterCount);
						ASSERT_FMOD_OK;

						for (int index = 0; index < parameterCount; ++index)
						{
							fmodResult = pAudioEvent->GetInstance()->getParameterByIndex(index, &pParameterInstance);
							ASSERT_FMOD_OK;

							FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
							fmodResult = pParameterInstance->getDescription(&parameterDescription);
							ASSERT_FMOD_OK;

							if (pSwitchState->name.compareNoCase(parameterDescription.name) == 0)
							{
								iter->second = index;
								fmodResult = pParameterInstance->setValue(pSwitchState->value);
								ASSERT_FMOD_OK;

								break;
							}
						}
					}
				}
				else
				{
					g_implLogger.Log(ELogType::Warning, "Trying to set an unknown Fmod switch: %s", pSwitchState->name.c_str());
				}
			}
		}

		auto const iter(m_audioSwitches.find(pSwitchState->eventPathId));

		if (iter != m_audioSwitches.end())
		{
			iter->second = pSwitchState;
		}
		else
		{
			m_audioSwitches.emplace(pSwitchState->eventPathId, pSwitchState);
		}
	}
	else
	{
		g_implLogger.Log(ELogType::Error, "Invalid AudioObjectData or RtpcData passed to the Fmod implementation of SetSwitchState");
		result = ERequestStatus::Failure;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CAudioObject::SetObstructionOcclusion(float const obstruction, float const occlusion)
{
	for (auto const pAudioEvent : m_audioEvents)
	{
		pAudioEvent->SetObstructionOcclusion(obstruction, occlusion);
	}

	m_obstruction = obstruction;
	m_occlusion = occlusion;

	return ERequestStatus::Success;
}
