// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Event.h"
#include "Common.h"
#include "Impl.h"
#include "BaseObject.h"
#include "BaseStandaloneFile.h"
#include "EventInstance.h"
#include "Listener.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

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
	auto* const pFmodEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(event);

	if (pFmodEventInstance != nullptr)
	{
		CEventInstance* pEventInstance = nullptr;
		FMOD_RESULT const fmodResult = pFmodEventInstance->getUserData(reinterpret_cast<void**>(&pEventInstance));
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if (pEventInstance != nullptr)
		{
			pEventInstance->SetToBeRemoved();
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK ProgrammerSoundCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* pEventInst, void* pInOutParameters)
{
	if (pEventInst != nullptr)
	{
		auto const pFmodEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(pEventInst);
		CEventInstance* pEventInstance = nullptr;
		FMOD_RESULT fmodResult = pFmodEventInstance->getUserData(reinterpret_cast<void**>(&pEventInstance));
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if ((pEventInstance != nullptr) && (pEventInstance->GetEvent() != nullptr))
		{
			if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
			{
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer during %s", __FUNCTION__);
				auto const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);
				char const* const szKey = pEventInstance->GetEvent()->GetKey().c_str();

				FMOD_STUDIO_SOUND_INFO soundInfo;
				fmodResult = CBaseObject::s_pSystem->getSoundInfo(szKey, &soundInfo);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				FMOD::Sound* pSound = nullptr;
				FMOD_MODE const mode = FMOD_CREATECOMPRESSEDSAMPLE | FMOD_NONBLOCKING | FMOD_3D | soundInfo.mode;
				fmodResult = CBaseStandaloneFile::s_pLowLevelSystem->createSound(soundInfo.name_or_data, mode, &soundInfo.exinfo, &pSound);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				pInOutProperties->sound = reinterpret_cast<FMOD_SOUND*>(pSound);
				pInOutProperties->subsoundIndex = soundInfo.subsoundindex;
			}
			else if (type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
			{
				CRY_ASSERT_MESSAGE(pInOutParameters != nullptr, "pInOutParameters is null pointer during %s", __FUNCTION__);
				auto const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				auto* pSound = reinterpret_cast<FMOD::Sound*>(pInOutProperties->sound);

				fmodResult = pSound->release();
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
			}
			else if ((type == FMOD_STUDIO_EVENT_CALLBACK_START_FAILED) || (type == FMOD_STUDIO_EVENT_CALLBACK_STOPPED))
			{
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				pEventInstance->SetToBeRemoved();
			}
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
CEvent::~CEvent()
{
	g_eventToParameterIndexes.erase(this);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CEvent::Execute(IObject* const pIObject, TriggerInstanceId const triggerInstanceId)
{
	ERequestStatus requestResult = ERequestStatus::Failure;

	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);

	switch (m_actionType)
	{
	case EActionType::Start:
		{
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, m_id, this, pBaseObject);
#else
			CEventInstance* const pEventInstance = g_pImpl->ConstructEventInstance(triggerInstanceId, m_id, this);
#endif        // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

			if (m_pEventDescription == nullptr)
			{
				fmodResult = CBaseObject::s_pSystem->getEventByID(&m_guid, &m_pEventDescription);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
			}

			if (m_pEventDescription != nullptr)
			{
				CRY_ASSERT(pEventInstance->GetFmodEventInstance() == nullptr);

				FMOD::Studio::EventInstance* pFmodEventInstance = nullptr;
				fmodResult = m_pEventDescription->createInstance(&pFmodEventInstance);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				pEventInstance->SetFmodEventInstance(pFmodEventInstance);
				pEventInstance->SetInternalParameters();

				if (m_hasProgrammerSound)
				{
					fmodResult = pEventInstance->GetFmodEventInstance()->setCallback(ProgrammerSoundCallback);
				}
				else
				{
					fmodResult = pEventInstance->GetFmodEventInstance()->setCallback(EventCallback, FMOD_STUDIO_EVENT_CALLBACK_START_FAILED | FMOD_STUDIO_EVENT_CALLBACK_STOPPED);
				}

				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				fmodResult = pEventInstance->GetFmodEventInstance()->setUserData(pEventInstance);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				fmodResult = pEventInstance->GetFmodEventInstance()->set3DAttributes(&pBaseObject->GetAttributes());
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				EventInstances& objectPendingEvents = pBaseObject->GetPendingEventInstances();
				CRY_ASSERT_MESSAGE(std::find(objectPendingEvents.begin(), objectPendingEvents.end(), pEventInstance) == objectPendingEvents.end(), "Event was already in the pending list during %s", __FUNCTION__);
				objectPendingEvents.push_back(pEventInstance);
				requestResult = ERequestStatus::Success;
			}

			break;
		}
	case EActionType::Stop:
		{
			pBaseObject->StopEventInstance(m_id);
			requestResult = ERequestStatus::SuccessDoNotTrack;

			break;
		}
	case EActionType::Pause: // Intentional fall-through.
	case EActionType::Resume:
		{
			FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

			bool const shouldPause = (m_actionType == EActionType::Pause);
			int const capacity = 32;
			EventInstances const& eventInstances = pBaseObject->GetEventInstances();

			for (auto const pEventInstance : eventInstances)
			{
				if (pEventInstance->GetId() == m_id)
				{
					if (m_pEventDescription == nullptr)
					{
						fmodResult = CBaseObject::s_pSystem->getEventByID(&m_guid, &m_pEventDescription);
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
					}

					if (m_pEventDescription != nullptr)
					{
						int count = 0;

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
						fmodResult = m_pEventDescription->getInstanceCount(&count);
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
						CRY_ASSERT_MESSAGE(count < capacity, "Instance count (%d) is higher or equal than array capacity (%d) during %s", count, capacity, __FUNCTION__);
#endif              // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

						FMOD::Studio::EventInstance* eventInstances[capacity];
						fmodResult = m_pEventDescription->getInstanceList(eventInstances, capacity, &count);
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

						for (int i = 0; i < count; ++i)
						{
							auto const pFmodEventInstance = eventInstances[i];

							if (pFmodEventInstance != nullptr)
							{
								fmodResult = pFmodEventInstance->setPaused(shouldPause);
								CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
							}
						}

						requestResult = ERequestStatus::SuccessDoNotTrack;
					}
				}
			}

			break;
		}
	}

	return requestResult;
}

//////////////////////////////////////////////////////////////////////////
void CEvent::Stop(IObject* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject*>(pIObject);
	pBaseObject->StopEventInstance(m_id);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
