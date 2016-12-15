// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioStandaloneFileManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>
#include <IAudioImpl.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioStandaloneFileManager::CAudioStandaloneFileManager(AudioStandaloneFileLookup& audioStandaloneFiles)
	: m_audioStandaloneFiles(audioStandaloneFiles)
	, m_audioStandaloneFilePool(g_audioCVars.m_audioStandaloneFilePoolSize, 1)
	, m_pImpl(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CAudioStandaloneFileManager::~CAudioStandaloneFileManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}

	if (!m_audioStandaloneFilePool.m_reserved.empty())
	{
		for (auto const pStandaloneFile : m_audioStandaloneFilePool.m_reserved)
		{
			CRY_ASSERT(pStandaloneFile->m_pImplData == nullptr);
			POOL_FREE(pStandaloneFile);
		}

		m_audioStandaloneFilePool.m_reserved.clear();
	}

	if (!m_audioStandaloneFiles.empty())
	{
		for (auto const& standaloneFilePair : m_audioStandaloneFiles)
		{
			CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;
			CRY_ASSERT(pStandaloneFile->m_pImplData == nullptr);
			POOL_FREE(pStandaloneFile);
		}

		m_audioStandaloneFiles.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	size_t const numPooledAudioStandaloneFiles = m_audioStandaloneFilePool.m_reserved.size();
	size_t const numActiveAudioStandaloneFiles = m_audioStandaloneFiles.size();

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	if ((numPooledAudioStandaloneFiles + numActiveAudioStandaloneFiles) > std::numeric_limits<size_t>::max())
	{
		CryFatalError("Exceeding numeric limits during CAudioStandaloneFileManager::Init");
	}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	size_t const numTotalAudioStandaloneFiles = numPooledAudioStandaloneFiles + numActiveAudioStandaloneFiles;
	CRY_ASSERT(!(numTotalAudioStandaloneFiles > 0 && numTotalAudioStandaloneFiles < m_audioStandaloneFilePool.m_reserveSize));

	if (m_audioStandaloneFilePool.m_reserveSize > numTotalAudioStandaloneFiles)
	{
		for (size_t i = 0; i < m_audioStandaloneFilePool.m_reserveSize - numActiveAudioStandaloneFiles; ++i)
		{
			AudioStandaloneFileId const id = m_audioStandaloneFilePool.GetNextID();
			POOL_NEW_CREATE(CATLStandaloneFile, pNewStandaloneFile)(id, eAudioDataScope_Global);
			m_audioStandaloneFilePool.m_reserved.push_back(pNewStandaloneFile);
		}
	}

	for (auto const pStandaloneFile : m_audioStandaloneFilePool.m_reserved)
	{
		CRY_ASSERT(pStandaloneFile->m_pImplData == nullptr);
		pStandaloneFile->m_pImplData = m_pImpl->NewAudioStandaloneFile();
	}

	for (auto const& standaloneFilePair : m_audioStandaloneFiles)
	{
		CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;
		CRY_ASSERT(pStandaloneFile->m_pImplData == nullptr);
		pStandaloneFile->m_pImplData = m_pImpl->NewAudioStandaloneFile();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Release()
{
	for (auto const pStandaloneFile : m_audioStandaloneFilePool.m_reserved)
	{
		m_pImpl->DeleteAudioStandaloneFile(pStandaloneFile->m_pImplData);
		pStandaloneFile->m_pImplData = nullptr;
	}

	for (auto const& standaloneFilePair : m_audioStandaloneFiles)
	{
		CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;
		m_pImpl->ResetAudioStandaloneFile(pStandaloneFile->m_pImplData);
		m_pImpl->DeleteAudioStandaloneFile(pStandaloneFile->m_pImplData);
		pStandaloneFile->m_pImplData = nullptr;
	}

	m_pImpl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
CATLStandaloneFile* CAudioStandaloneFileManager::GetStandaloneFile(char const* const szFile)
{
	CATLStandaloneFile* pStandaloneFile = nullptr;

	if (!m_audioStandaloneFilePool.m_reserved.empty())
	{
		pStandaloneFile = m_audioStandaloneFilePool.m_reserved.back();
		m_audioStandaloneFilePool.m_reserved.pop_back();
	}
	else
	{
		AudioStandaloneFileId const instanceID = m_audioStandaloneFilePool.GetNextID();
		POOL_NEW(CATLStandaloneFile, pStandaloneFile)(instanceID, eAudioDataScope_Global);

		if (pStandaloneFile != nullptr)
		{
			pStandaloneFile->m_pImplData = m_pImpl->NewAudioStandaloneFile();
		}
		else
		{
			--m_audioStandaloneFilePool.m_idCounter;
			g_audioLogger.Log(eAudioLogType_Warning, "Failed to create a new instance of a standalone file during CAudioStandaloneFileManager::GetStandaloneFile!");
		}
	}

	if (pStandaloneFile != nullptr)
	{
		pStandaloneFile->m_id = static_cast<AudioStandaloneFileId>(AudioStringToId(szFile));
		m_audioStandaloneFiles[pStandaloneFile->GetId()] = pStandaloneFile;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pStandaloneFile->m_name = szFile;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}

	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
CATLStandaloneFile* CAudioStandaloneFileManager::LookupId(AudioStandaloneFileId const instanceId) const
{
	CATLStandaloneFile* pStandaloneFile = nullptr;

	if (instanceId != INVALID_AUDIO_STANDALONE_FILE_ID)
	{
		AudioStandaloneFileLookup::const_iterator const iter(m_audioStandaloneFiles.find(instanceId));

		if (iter != m_audioStandaloneFiles.end())
		{
			pStandaloneFile = iter->second;
		}
	}

	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		m_audioStandaloneFiles.erase(pStandaloneFile->GetId());
		pStandaloneFile->Clear();

		m_pImpl->ResetAudioStandaloneFile(pStandaloneFile->m_pImplData);
		if (m_audioStandaloneFilePool.m_reserved.size() < m_audioStandaloneFilePool.m_reserveSize)
		{
			m_audioStandaloneFilePool.m_reserved.push_back(pStandaloneFile);
		}
		else
		{
			m_pImpl->DeleteAudioStandaloneFile(pStandaloneFile->m_pImplData);
			POOL_FREE(pStandaloneFile);
		}
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

	auxGeom.Draw2dLabel(posX, posY, 1.6f, headerColor, false, "Standalone Files [%" PRISIZE_T "]", m_audioStandaloneFiles.size());
	posX += 20.0f;
	posY += 17.0f;

	for (auto const& standaloneFilePair : m_audioStandaloneFiles)
	{
		CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;

		float const* pColor = itemOtherColor;

		switch (pStandaloneFile->m_state)
		{
		case eAudioStandaloneFileState_Playing:
			{
				pColor = itemPlayingColor;

				break;
			}
		case eAudioStandaloneFileState_Loading:
			{
				pColor = itemLoadingColor;

				break;
			}
		case eAudioStandaloneFileState_Stopping:
			{
				pColor = itemStoppingColor;

				break;
			}
		}

		auxGeom.Draw2dLabel(posX, posY, 1.2f,
		                    pColor,
		                    false,
		                    "%s on %s : %u",
		                    pStandaloneFile->m_name.c_str(),
		                    pStandaloneFile->m_pAudioObject->m_name.c_str(),
		                    pStandaloneFile->GetId());

		posY += 10.0f;
	}
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
