// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "StandaloneFile.h"

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

#include <fmod_common.h>
#include <fmod.hpp>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
FMOD_RESULT F_CALLBACK FileCallback(FMOD_CHANNELCONTROL* pChannelControl, FMOD_CHANNELCONTROL_TYPE controlType, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType, void* pData1, void* pData2)
{
	if (controlType == FMOD_CHANNELCONTROL_CHANNEL && callbackType == FMOD_CHANNELCONTROL_CALLBACK_END)
	{

		FMOD::Channel* pChannel = (FMOD::Channel*)pChannelControl;
		CStandaloneFile* pStandaloneFile = nullptr;
		FMOD_RESULT const fmodResult = pChannel->getUserData(reinterpret_cast<void**>(&pStandaloneFile));
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if (pStandaloneFile != nullptr)
		{
			pStandaloneFile->m_pChannel = nullptr;
			pStandaloneFile->ReportFileFinished();
		}
	}

	return FMOD_OK;
}

//////////////////////////////////////////////////////////////////////////
void CStandaloneFile::StartLoading()
{
	FMOD_RESULT fmodResult = g_pLowLevelSystem->createSound(m_fileName, FMOD_CREATESTREAM | FMOD_NONBLOCKING | FMOD_3D, nullptr, &m_pLowLevelSound);

	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
}

//////////////////////////////////////////////////////////////////////////
bool CStandaloneFile::IsReady()
{
	FMOD_OPENSTATE state = FMOD_OPENSTATE_ERROR;

	if (m_pLowLevelSound)
	{
		m_pLowLevelSound->getOpenState(&state, nullptr, nullptr, nullptr);

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
		if (state == FMOD_OPENSTATE_ERROR)
		{
			Cry::Audio::Log(ELogType::Error, "Failed to load audio file %s", m_fileName.c_str());
		}
#endif    // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	}

	return state != FMOD_OPENSTATE_LOADING;
}

//////////////////////////////////////////////////////////////////////////
void CStandaloneFile::PlayFile(FMOD_3D_ATTRIBUTES const& attributes)
{
	FMOD_RESULT const fmodResult = g_pLowLevelSystem->playSound(m_pLowLevelSound, nullptr, true, &m_pChannel);
	CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

	if (m_pChannel != nullptr)
	{
		m_pChannel->setUserData(this);
		m_pChannel->set3DAttributes(&attributes.position, &attributes.velocity);
		m_pChannel->setCallback(FileCallback);
		m_pChannel->setPaused(false);
	}

	ReportFileStarted();
}

//////////////////////////////////////////////////////////////////////////
void CStandaloneFile::Set3DAttributes(FMOD_3D_ATTRIBUTES const& attributes)
{
	m_pChannel->set3DAttributes(&attributes.position, &attributes.velocity);
}

//////////////////////////////////////////////////////////////////////////
void CStandaloneFile::StopFile()
{
	if (m_pChannel != nullptr)
	{
		FMOD_RESULT const fmodResult = m_pChannel->stop();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
	}
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio