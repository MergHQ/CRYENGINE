// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ProgrammerSoundFile.h"
#include "BaseObject.h"

#include <fmod_common.h>
#include <fmod.hpp>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK ProgrammerSoundFileCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* pEvent, void* pInOutParameters)
{
	if (pEvent != nullptr)
	{
		FMOD::Studio::EventInstance* const pEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(pEvent);
		CProgrammerSoundFile* pFile = nullptr;
		FMOD_RESULT fmodResult = pEventInstance->getUserData(reinterpret_cast<void**>(&pFile));
		ASSERT_FMOD_OK;

		if (pFile != nullptr)
		{
			if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
			{
				CRY_ASSERT(pInOutParameters);
				FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES* const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				// Create the sound
				FMOD::Sound* pSound = nullptr;
				fmodResult = pFile->s_pLowLevelSystem->createSound(pFile->GetFileName(), FMOD_CREATESTREAM | FMOD_NONBLOCKING | FMOD_3D, nullptr, &pSound);
				ASSERT_FMOD_OK;

				// Pass the sound to FMOD
				pInOutProperties->sound = reinterpret_cast<FMOD_SOUND*>(pSound);
			}
			else if (type == FMOD_STUDIO_EVENT_CALLBACK_DESTROY_PROGRAMMER_SOUND)
			{
				CRY_ASSERT(pInOutParameters);
				FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES* const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				// Obtain the sound
				FMOD::Sound* pSound = reinterpret_cast<FMOD::Sound*>(pInOutProperties->sound);

				// Release the sound
				fmodResult = pSound->release();
				ASSERT_FMOD_OK;
			}
			else if (type == FMOD_STUDIO_EVENT_CALLBACK_STOPPED)
			{
				pFile->ReportFileFinished();
			}
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::StartLoading()
{
	FMOD::Studio::EventDescription* pEventDescription = nullptr;
	FMOD_RESULT fmodResult = CBaseObject::s_pSystem->getEventByID(&m_eventGuid, &pEventDescription);
	ASSERT_FMOD_OK;

	fmodResult = pEventDescription->createInstance(&m_pEventInstance);
	ASSERT_FMOD_OK;

	fmodResult = m_pEventInstance->setUserData(this);
	ASSERT_FMOD_OK;

	fmodResult = m_pEventInstance->setCallback(ProgrammerSoundFileCallback);
	ASSERT_FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
bool CProgrammerSoundFile::IsReady()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::PlayFile(FMOD_3D_ATTRIBUTES const& attributes)
{
	FMOD_RESULT fmodResult = m_pEventInstance->start();
	ASSERT_FMOD_OK;

	fmodResult = m_pEventInstance->set3DAttributes(&attributes);
	ASSERT_FMOD_OK;

}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes)
{
	FMOD_RESULT fmodResult = m_pEventInstance->set3DAttributes(&attributes);
	ASSERT_FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::StopFile()
{
	FMOD_RESULT fmodResult = m_pEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
	ASSERT_FMOD_OK;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio