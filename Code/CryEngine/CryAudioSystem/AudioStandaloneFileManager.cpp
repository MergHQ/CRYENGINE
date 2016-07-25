// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioStandaloneFileManager.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>
#include <IAudioImpl.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioStandaloneFileManager::CAudioStandaloneFileManager()
	: m_audioStandaloneFilePool(g_audioCVars.m_audioStandaloneFilePoolSize, 1)
	, m_pImpl(nullptr)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	, m_pDebugNameStore(nullptr)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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

	if (!m_activeAudioStandaloneFiles.empty())
	{
		for (auto const& standaloneFilePair : m_activeAudioStandaloneFiles)
		{
			CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;
			CRY_ASSERT(pStandaloneFile->m_pImplData == nullptr);
			POOL_FREE(pStandaloneFile);
		}

		m_activeAudioStandaloneFiles.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	size_t const numPooledAudioStandaloneFiles = m_audioStandaloneFilePool.m_reserved.size();
	size_t const numActiveAudioStandaloneFiles = m_activeAudioStandaloneFiles.size();

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

	for (auto const& standaloneFilePair : m_activeAudioStandaloneFiles)
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

	for (auto const& standaloneFilePair : m_activeAudioStandaloneFiles)
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
		m_activeAudioStandaloneFiles[pStandaloneFile->GetId()] = pStandaloneFile;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_pDebugNameStore->AddAudioStandaloneFile(pStandaloneFile->m_id, szFile);
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
		ActiveStandaloneFilesMap::const_iterator const Iter(m_activeAudioStandaloneFiles.find(instanceId));

		if (Iter != m_activeAudioStandaloneFiles.end())
		{
			pStandaloneFile = Iter->second;
		}
	}

	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		m_activeAudioStandaloneFiles.erase(pStandaloneFile->GetId());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		m_pDebugNameStore->RemoveAudioStandaloneFile(pStandaloneFile->m_id);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

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

	auxGeom.Draw2dLabel(posX, posY, 1.6f, headerColor, false, "Standalone Files [%" PRISIZE_T "]", m_activeAudioStandaloneFiles.size());
	posX += 20.0f;
	posY += 17.0f;

	for (auto const& standaloneFilePair : m_activeAudioStandaloneFiles)
	{
		CATLStandaloneFile* const pStandaloneFile = standaloneFilePair.second;
		char const* const szName = m_pDebugNameStore->LookupAudioStandaloneFileName(pStandaloneFile->m_id);

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
		                    szName,
		                    m_pDebugNameStore->LookupAudioObjectName(pStandaloneFile->m_audioObjectId),
		                    pStandaloneFile->GetId());

		posY += 10.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
{
	m_pDebugNameStore = pDebugNameStore;
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
