// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioStandaloneFileManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include <IAudioImpl.h>
#include <CryString/HashedString.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

using namespace CryAudio;
using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioStandaloneFileManager::~CAudioStandaloneFileManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Release()
{
	if (!m_constructedStandaloneFiles.empty())
	{
		for (auto pStandaloneFile : m_constructedStandaloneFiles)
		{
			m_pImpl->DestructAudioStandaloneFile(pStandaloneFile->m_pImplData);
			delete pStandaloneFile;
		}
		m_constructedStandaloneFiles.clear();
	}

	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CATLStandaloneFile* CAudioStandaloneFileManager::ConstructStandaloneFile(char const* const szFile, bool const bLocalized, IAudioTrigger const* const pTriggerImpl)
{
	CATLStandaloneFile* pStandaloneFile = new CATLStandaloneFile();

	pStandaloneFile->m_pImplData = m_pImpl->ConstructAudioStandaloneFile(*pStandaloneFile, szFile, bLocalized, pTriggerImpl);
	pStandaloneFile->m_hashedFilename = CHashedString(szFile);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pStandaloneFile->m_bLocalized = bLocalized;
	pStandaloneFile->m_pTrigger = pTriggerImpl;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_constructedStandaloneFiles.push_back(pStandaloneFile);
	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		m_constructedStandaloneFiles.remove(pStandaloneFile);
		m_pImpl->DestructAudioStandaloneFile(pStandaloneFile->m_pImplData);
		delete pStandaloneFile;
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const
{
	static float const headerColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
	static float const itemPlayingColor[4] = { 0.1f, 0.6f, 0.1f, 0.9f };
	static float const itemStoppingColor[4] = { 0.8f, 0.7f, 0.1f, 0.9f };
	static float const itemLoadingColor[4] = { 0.9f, 0.2f, 0.2f, 0.9f };
	static float const itemOtherColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };

	auxGeom.Draw2dLabel(posX, posY, 1.6f, headerColor, false, "Standalone Files [%" PRISIZE_T "]", m_constructedStandaloneFiles.size());
	posX += 20.0f;
	posY += 17.0f;

	for (auto pStandaloneFile : m_constructedStandaloneFiles)
	{
		float const* pColor = itemOtherColor;

		switch (pStandaloneFile->m_state)
		{
		case EAudioStandaloneFileState::Playing:
			{
				pColor = itemPlayingColor;

				break;
			}
		case EAudioStandaloneFileState::Loading:
			{
				pColor = itemLoadingColor;

				break;
			}
		case EAudioStandaloneFileState::Stopping:
			{
				pColor = itemStoppingColor;

				break;
			}
		}

		auxGeom.Draw2dLabel(posX, posY, 1.2f,
		                    pColor,
		                    false,
		                    "%s on %s",
		                    pStandaloneFile->m_hashedFilename.GetText().c_str(),
		                    pStandaloneFile->m_pAudioObject->m_name.c_str());

		posY += 10.0f;
	}
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
