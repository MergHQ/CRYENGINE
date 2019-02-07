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
FMOD_RESULT F_CALLBACK ProgrammerSoundFileCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* pEventInstance, void* pInOutParameters)
{
	if (pEventInstance != nullptr)
	{
		auto const pFmodEventInstance = reinterpret_cast<FMOD::Studio::EventInstance*>(pEventInstance);
		CProgrammerSoundFile* pFile = nullptr;
		FMOD_RESULT fmodResult = pFmodEventInstance->getUserData(reinterpret_cast<void**>(&pFile));
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if (pFile != nullptr)
		{
			if (type == FMOD_STUDIO_EVENT_CALLBACK_CREATE_PROGRAMMER_SOUND)
			{
				CRY_ASSERT(pInOutParameters);
				FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES* const pInOutProperties = reinterpret_cast<FMOD_STUDIO_PROGRAMMER_SOUND_PROPERTIES*>(pInOutParameters);

				// Create the sound
				FMOD::Sound* pSound = nullptr;
				fmodResult = g_pLowLevelSystem->createSound(pFile->GetFileName(), FMOD_CREATESTREAM | FMOD_NONBLOCKING | FMOD_3D, nullptr, &pSound);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

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
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
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
	FMOD_RESULT fmodResult = g_pSystem->getEventByID(&m_eventGuid, &pEventDescription);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	fmodResult = pEventDescription->createInstance(&m_pEventInstance);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	fmodResult = m_pEventInstance->setUserData(this);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	fmodResult = m_pEventInstance->setCallback(ProgrammerSoundFileCallback);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
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
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	fmodResult = m_pEventInstance->set3DAttributes(&attributes);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes)
{
	FMOD_RESULT fmodResult = m_pEventInstance->set3DAttributes(&attributes);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
}

//////////////////////////////////////////////////////////////////////////
void CProgrammerSoundFile::StopFile()
{
	FMOD_RESULT fmodResult = m_pEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio