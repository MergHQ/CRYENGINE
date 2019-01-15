// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Trigger.h"
#include "Impl.h"
#include "BaseObject.h"
#include "BaseStandaloneFile.h"
#include "Event.h"
#include "Listener.h"

#include <Logger.h>

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
			pEvent->SetToBeRemoved();
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
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer during %s", __FUNCTION__);
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
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer during %s", __FUNCTION__);
				auto const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				auto* pSound = reinterpret_cast<FMOD::Sound*>(pInOutProperties->sound);

				fmodResult = pSound->release();
				ASSERT_FMOD_OK;
			}
			else if ((type == FMOD_STUDIO_EVENT_CALLBACK_START_FAILED) || (type == FMOD_STUDIO_EVENT_CALLBACK_STOPPED))
			{
				ASSERT_FMOD_OK;
				pEvent->SetToBeRemoved();
			}
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CTrigger::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);

		switch (m_actionType)
		{
		case EActionType::Start:
			{
				FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

				CEvent* const pEvent = g_pImpl->ConstructEvent(triggerInstanceId);
				pEvent->SetTrigger(this);

				if (m_pEventDescription == nullptr)
				{
					fmodResult = CBaseObject::s_pSystem->getEventByID(&m_guid, &m_pEventDescription);
					ASSERT_FMOD_OK;
				}

				if (m_pEventDescription != nullptr)
				{
					CRY_ASSERT(pEvent->GetInstance() == nullptr);

					FMOD::Studio::EventInstance* pInstance = nullptr;
					fmodResult = m_pEventDescription->createInstance(&pInstance);
					ASSERT_FMOD_OK;
					pEvent->SetInstance(pInstance);
					pEvent->SetInternalParameters();

					if (m_hasProgrammerSound)
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
					fmodResult = pEvent->GetInstance()->set3DAttributes(&pObject->GetAttributes());
					ASSERT_FMOD_OK;

					CRY_ASSERT(pEvent->GetId() == InvalidCRC32);
					pEvent->SetId(m_id);
					pEvent->SetObject(pObject);

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
					pEvent->SetName(m_name.c_str());
#endif          // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

					Events& objectPendingEvents = pObject->GetPendingEvents();
					CRY_ASSERT_MESSAGE(std::find(objectPendingEvents.begin(), objectPendingEvents.end(), pEvent) == objectPendingEvents.end(), "Event was already in the pending list during %s", __FUNCTION__);
					objectPendingEvents.push_back(pEvent);
					requestResult = ERequestStatus::Success;
				}

				break;
			}
		case EActionType::Stop:
			{
				pObject->StopEvent(m_id);
				requestResult = ERequestStatus::SuccessDoNotTrack;

				break;
			}
		case EActionType::Pause: // Intentional fall-through.
		case EActionType::Resume:
			{
				FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

				bool const shouldPause = (m_actionType == EActionType::Pause);
				int const capacity = 32;
				Events const& events = pObject->GetEvents();

				for (auto const pEvent : events)
				{
					if (pEvent->GetId() == m_id)
					{
						if (m_pEventDescription == nullptr)
						{
							fmodResult = CBaseObject::s_pSystem->getEventByID(&m_guid, &m_pEventDescription);
							ASSERT_FMOD_OK;
						}

						if (m_pEventDescription != nullptr)
						{
							int count = 0;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
							fmodResult = m_pEventDescription->getInstanceCount(&count);
							ASSERT_FMOD_OK;
							CRY_ASSERT_MESSAGE(count < capacity, "Instance count (%d) is higher or equal than array capacity (%d) during %s", count, capacity, __FUNCTION__);
#endif              // INCLUDE_FMOD_IMPL_PRODUCTION_CODE

							FMOD::Studio::EventInstance* eventInstances[capacity];
							fmodResult = m_pEventDescription->getInstanceList(eventInstances, capacity, &count);
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
				}

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid object or event pointer passed to the Fmod implementation of %s.", __FUNCTION__);
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CTrigger::Stop(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pObject = static_cast<CBaseObject*>(pIObject);
		pObject->StopEvent(m_id);
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
