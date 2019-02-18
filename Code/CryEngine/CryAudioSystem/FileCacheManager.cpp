// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "FileCacheManager.h"
#include "Common.h"
#include "FileEntry.h"
#include "PreloadRequest.h"
#include "CVars.h"
#include "Common/FileInfo.h"
#include "Common/IImpl.h"
#include <CryRenderer/IRenderer.h>
#include <CryString/CryPath.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Common/Logger.h"
	#include "Debug.h"
	#include "Common/DebugStyle.h"
	#include <CryRenderer/IRenderAuxGeom.h>
	#include <CrySystem/ITimer.h>
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
enum class EFileCacheManagerDebugFilter : EnumFlagsType
{
	All            = 0,
	Globals        = BIT(6), // a
	LevelSpecifics = BIT(7), // b
	UseCounted     = BIT(8), // c
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EFileCacheManagerDebugFilter);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
CFileCacheManager::~CFileCacheManager()
{
	CRY_ASSERT_MESSAGE(m_fileEntries.empty(), "There are still file entries during %s", __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::Initialize()
{
#if CRY_PLATFORM_DURANGO
	m_pMemoryHeap = gEnv->pSystem->GetIMemoryManager()->CreateCustomMemoryHeapInstance(IMemoryManager::eapAPU);

	if (m_pMemoryHeap != nullptr)
	{
		m_maxByteTotal = static_cast<size_t>(g_cvars.m_fileCacheManagerSize) << 10;
	}
#else
	m_pMemoryHeap = gEnv->pSystem->GetIMemoryManager()->CreateCustomMemoryHeapInstance(IMemoryManager::eapCustomAlignment);
#endif // CRY_PLATFORM_DURANGO
}

//////////////////////////////////////////////////////////////////////////
FileEntryId CFileCacheManager::TryAddFileCacheEntry(XmlNodeRef const pFileNode, EDataScope const dataScope, bool const bAutoLoad)
{
	FileEntryId fileEntryId = InvalidFileEntryId;
	Impl::SFileInfo fileInfo;

	if (g_pIImpl->ConstructFile(pFileNode, &fileInfo) == ERequestStatus::Success)
	{
		CryFixedStringT<MaxFilePathLength> fullPath(g_pIImpl->GetFileLocation(&fileInfo));
		fullPath += "/";
		fullPath += fileInfo.szFileName;
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CFileEntry");
		auto pFileEntry = new CFileEntry(fullPath, fileInfo.pImplData);

		if (pFileEntry != nullptr)
		{
			pFileEntry->m_memoryBlockAlignment = fileInfo.memoryBlockAlignment;

			if (fileInfo.bLocalized)
			{
				pFileEntry->m_flags |= EFileFlags::Localized;
			}

			fileEntryId = static_cast<FileEntryId>(StringToId(pFileEntry->m_path.c_str()));
			CFileEntry* const __restrict pExisitingFileEntry = stl::find_in_map(m_fileEntries, fileEntryId, nullptr);

			if (pExisitingFileEntry == nullptr)
			{
				if (!bAutoLoad)
				{
					// Can now be ref-counted and therefore manually unloaded.
					pFileEntry->m_flags |= EFileFlags::UseCounted;
				}

				pFileEntry->m_dataScope = dataScope;
				pFileEntry->m_path.MakeLower();
				size_t const fileSize = gEnv->pCryPak->FGetSize(pFileEntry->m_path.c_str());

				if (fileSize > 0)
				{
					pFileEntry->m_size = fileSize;
					pFileEntry->m_flags = (pFileEntry->m_flags | EFileFlags::NotCached) & ~EFileFlags::NotFound;
					pFileEntry->m_streamTaskType = eStreamTaskTypeSound;
				}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
				else
				{
					Cry::Audio::Log(ELogType::Warning, "Couldn't find audio file %s for pre-loading.", pFileEntry->m_path.c_str());
				}
#endif    // CRY_AUDIO_USE_PRODUCTION_CODE

				m_fileEntries[fileEntryId] = pFileEntry;
			}
			else
			{
				if (((pExisitingFileEntry->m_flags & EFileFlags::UseCounted) != 0) && bAutoLoad)
				{
					// This file entry is upgraded from "manual loading" to "auto loading" but needs a reset to "manual loading" again!
					pExisitingFileEntry->m_flags = (pExisitingFileEntry->m_flags | EFileFlags::NeedsResetToManualLoading) & ~EFileFlags::UseCounted;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
					Cry::Audio::Log(ELogType::Always, R"(Upgraded file entry from "manual loading" to "auto loading": %s)", pExisitingFileEntry->m_path.c_str());
#endif      // CRY_AUDIO_USE_PRODUCTION_CODE
				}

				// Entry already exists, free the memory!
				g_pIImpl->DestructFile(pFileEntry->m_pImplData);
				delete pFileEntry;
			}
		}
	}

	return fileEntryId;
}

//////////////////////////////////////////////////////////////////////////
bool CFileCacheManager::TryRemoveFileCacheEntry(FileEntryId const id, EDataScope const dataScope)
{
	bool bSuccess = false;
	FileEntries::iterator const iter(m_fileEntries.find(id));

	if (iter != m_fileEntries.end())
	{
		CFileEntry* const pFileEntry = iter->second;

		if (pFileEntry->m_dataScope == dataScope)
		{
			UncacheFileCacheEntryInternal(pFileEntry, true, true);
			g_pIImpl->DestructFile(pFileEntry->m_pImplData);
			delete pFileEntry;
			m_fileEntries.erase(iter);
		}
		else if ((dataScope == EDataScope::LevelSpecific) && ((pFileEntry->m_flags & EFileFlags::NeedsResetToManualLoading) != 0))
		{
			pFileEntry->m_flags = (pFileEntry->m_flags | EFileFlags::UseCounted) & ~EFileFlags::NeedsResetToManualLoading;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Always, R"(Downgraded file entry from "auto loading" to "manual loading": %s)", pFileEntry->m_path.c_str());
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::UpdateLocalizedFileCacheEntries()
{
	for (auto const& audioFileEntryPair : m_fileEntries)
	{
		CFileEntry* const pFileEntry = audioFileEntryPair.second;

		if (pFileEntry != nullptr && (pFileEntry->m_flags & EFileFlags::Localized) != 0)
		{
			if ((pFileEntry->m_flags & (EFileFlags::Cached | EFileFlags::Loading)) != 0)
			{
				// The file needs to be unloaded first.
				size_t const useCount = pFileEntry->m_useCount;
				pFileEntry->m_useCount = 0;   // Needed to uncache without an error.
				UncacheFile(pFileEntry);
				UpdateLocalizedFileEntryData(pFileEntry);
				TryCacheFileCacheEntryInternal(pFileEntry, audioFileEntryPair.first, true, true, useCount);
			}
			else
			{
				// The file is not cached or loading, it is safe to update the corresponding CFileEntry data.
				UpdateLocalizedFileEntryData(pFileEntry);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CFileCacheManager::TryLoadRequest(PreloadRequestId const preloadRequestId, bool const bLoadSynchronously, bool const bAutoLoadOnly)
{
	bool bFullSuccess = false;
	bool bFullFailure = true;
	CPreloadRequest* const pPreloadRequest = stl::find_in_map(g_preloadRequests, preloadRequestId, nullptr);

	if (pPreloadRequest != nullptr && (!bAutoLoadOnly || (bAutoLoadOnly && pPreloadRequest->m_bAutoLoad)))
	{
		bFullSuccess = true;

		for (FileEntryId const fileEntryId : pPreloadRequest->m_fileEntryIds)
		{
			CFileEntry* const pFileEntry = stl::find_in_map(m_fileEntries, fileEntryId, nullptr);

			if (pFileEntry != nullptr)
			{
				bool const bTemp = TryCacheFileCacheEntryInternal(pFileEntry, fileEntryId, bLoadSynchronously);
				bFullSuccess = bFullSuccess && bTemp;
				bFullFailure = bFullFailure && !bTemp;
			}
		}
	}

	return (bFullSuccess) ? ERequestStatus::Success : ((bFullFailure) ? ERequestStatus::Failure : ERequestStatus::PartialSuccess);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CFileCacheManager::TryUnloadRequest(PreloadRequestId const preloadRequestId)
{
	bool bFullSuccess = false;
	bool bFullFailure = true;
	CPreloadRequest* const pPreloadRequest = stl::find_in_map(g_preloadRequests, preloadRequestId, nullptr);

	if (pPreloadRequest != nullptr)
	{
		bFullSuccess = true;

		for (FileEntryId const fileEntryId : pPreloadRequest->m_fileEntryIds)
		{
			CFileEntry* const pFileEntry = stl::find_in_map(m_fileEntries, fileEntryId, nullptr);

			if (pFileEntry != nullptr)
			{
				bool const bTemp = UncacheFileCacheEntryInternal(pFileEntry, false);
				bFullSuccess = bFullSuccess && bTemp;
				bFullFailure = bFullFailure && !bTemp;
			}
		}
	}

	return (bFullSuccess) ? ERequestStatus::Success : ((bFullFailure) ? ERequestStatus::Failure : ERequestStatus::PartialSuccess);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CFileCacheManager::UnloadDataByScope(EDataScope const dataScope)
{
	FileEntries::iterator iter(m_fileEntries.begin());
	FileEntries::const_iterator iterEnd(m_fileEntries.end());

	while (iter != iterEnd)
	{
		CFileEntry* const pFileEntry = iter->second;

		if (pFileEntry != nullptr && pFileEntry->m_dataScope == dataScope)
		{
			if (UncacheFileCacheEntryInternal(pFileEntry, true, true))
			{
				g_pIImpl->DestructFile(pFileEntry->m_pImplData);
				delete pFileEntry;
				iter = m_fileEntries.erase(iter);
				iterEnd = m_fileEntries.end();
				continue;
			}
		}

		++iter;
	}

	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
bool CFileCacheManager::UncacheFileCacheEntryInternal(CFileEntry* const pFileEntry, bool const bNow, bool const bIgnoreUsedCount /* = false */)
{
	bool bSuccess = false;

	// In any case decrement the used count.
	if (pFileEntry->m_useCount > 0)
	{
		--pFileEntry->m_useCount;
	}

	if (pFileEntry->m_useCount < 1 || bIgnoreUsedCount)
	{
		// Must be cached to proceed.
		if ((pFileEntry->m_flags & EFileFlags::Cached) != 0)
		{
			// Only "use-counted" files can become removable!
			if ((pFileEntry->m_flags & EFileFlags::UseCounted) != 0)
			{
				pFileEntry->m_flags |= EFileFlags::Removable;
			}

			if (bNow || bIgnoreUsedCount)
			{
				UncacheFile(pFileEntry);
			}
		}
		else if ((pFileEntry->m_flags & EFileFlags::Loading) != 0)
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Always, "Trying to remove a loading file cache entry %s", pFileEntry->m_path.c_str());
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			// Abort loading and reset the entry.
			UncacheFile(pFileEntry);
		}
		else if ((pFileEntry->m_flags & EFileFlags::MemAllocFail) != 0)
		{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Always, "Resetting a memalloc-failed file cache entry %s", pFileEntry->m_path.c_str());
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			// Only reset the entry.
			pFileEntry->m_flags = (pFileEntry->m_flags | EFileFlags::NotCached) & ~EFileFlags::MemAllocFail;
		}

		// The file was either properly uncached, queued for uncache or not cached at all.
		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError)
{
	// We "user abort" quite frequently so this is not something we want to assert on.
	CRY_ASSERT(nError == 0 || nError == ERROR_USER_ABORT);

	FinishStreamInternal(pStream, nError);
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY)
{
	#if CRY_PLATFORM_DURANGO
	auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "FileCacheManager (%d of %d KiB) [Entries: %d]", static_cast<int>(m_currentByteTotal >> 10), static_cast<int>(m_maxByteTotal >> 10), static_cast<int>(m_fileEntries.size()));
	#else
	auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "FileCacheManager (%d KiB) [Entries: %d]", static_cast<int>(m_currentByteTotal >> 10), static_cast<int>(m_fileEntries.size()));
	#endif // CRY_PLATFORM_DURANGO

	posY += Debug::g_listHeaderLineHeight;

	if (!m_fileEntries.empty())
	{
		float const frameTime = gEnv->pTimer->GetAsyncTime().GetSeconds();

		CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
		lowerCaseSearchString.MakeLower();

		bool const drawAll = (g_cvars.m_fileCacheManagerDebugFilter == EFileCacheManagerDebugFilter::All);
		bool const drawUseCounted = ((g_cvars.m_fileCacheManagerDebugFilter & EFileCacheManagerDebugFilter::UseCounted) != 0);
		bool const drawGlobals = (g_cvars.m_fileCacheManagerDebugFilter & EFileCacheManagerDebugFilter::Globals) != 0;
		bool const drawLevelSpecifics = (g_cvars.m_fileCacheManagerDebugFilter & EFileCacheManagerDebugFilter::LevelSpecifics) != 0;

		std::vector<CryFixedStringT<MaxFilePathLength>> fileNamesSorted; // Create list to sort file names alphabetically.
		fileNamesSorted.reserve(m_fileEntries.size());

		for (auto const& file : m_fileEntries)
		{
			fileNamesSorted.emplace_back(file.second->m_path.c_str());
		}

		std::sort(fileNamesSorted.begin(), fileNamesSorted.end());

		for (auto const& fileName : fileNamesSorted)
		{
			for (auto const& audioFileEntryPair : m_fileEntries)
			{
				if (audioFileEntryPair.second->m_path == fileName)
				{
					CFileEntry* const pFileEntry = audioFileEntryPair.second;

					char const* const szFileName = PathUtil::GetFileName(pFileEntry->m_path);
					CryFixedStringT<MaxControlNameLength> lowerCaseFileName(szFileName);
					lowerCaseFileName.MakeLower();

					if ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseFileName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos))
					{
						bool draw = false;
						ColorF color;

						if (pFileEntry->m_dataScope == EDataScope::Global)
						{
							draw = (drawAll || drawGlobals) || (drawUseCounted && ((pFileEntry->m_flags & EFileFlags::UseCounted) != 0));
							color = Debug::s_afcmColorScopeGlobal;
						}
						else if (pFileEntry->m_dataScope == EDataScope::LevelSpecific)
						{
							draw = (drawAll || drawLevelSpecifics) || (drawUseCounted && (pFileEntry->m_flags & EFileFlags::UseCounted) != 0);
							color = Debug::s_afcmColorScopeLevelSpecific;
						}

						if (draw)
						{
							if ((pFileEntry->m_flags & EFileFlags::Loading) != 0)
							{
								color = Debug::s_afcmColorFileLoading;
							}
							else if ((pFileEntry->m_flags & EFileFlags::MemAllocFail) != 0)
							{
								color = Debug::s_afcmColorFileMemAllocFail;
							}
							else if ((pFileEntry->m_flags & EFileFlags::Removable) != 0)
							{
								color = Debug::s_afcmColorFileRemovable;
							}
							else if ((pFileEntry->m_flags & EFileFlags::NotCached) != 0)
							{
								color = Debug::s_afcmColorFileNotCached;
							}
							else if ((pFileEntry->m_flags & EFileFlags::NotFound) != 0)
							{
								color = Debug::s_afcmColorFileNotFound;
							}

							float const ratio = (frameTime - pFileEntry->m_timeCached) / 2.0f;

							if (ratio <= 1.0f)
							{
								color[0] *= clamp_tpl(ratio, 0.0f, color[0]);
								color[1] *= clamp_tpl(ratio, 0.0f, color[1]);
								color[2] *= clamp_tpl(ratio, 0.0f, color[2]);
							}

							CryFixedStringT<MaxMiscStringLength> debugText;

							if ((pFileEntry->m_flags & EFileFlags::UseCounted) != 0)
							{
								if (pFileEntry->m_size < 1024)
								{
									debugText.Format(debugText + "%s (%" PRISIZE_T " Byte) [%" PRISIZE_T "]", pFileEntry->m_path.c_str(), pFileEntry->m_size, pFileEntry->m_useCount);
								}
								else
								{
									debugText.Format(debugText + "%s (%" PRISIZE_T " KiB) [%" PRISIZE_T "]", pFileEntry->m_path.c_str(), pFileEntry->m_size >> 10, pFileEntry->m_useCount);
								}
							}
							else
							{
								if (pFileEntry->m_size < 1024)
								{
									debugText.Format(debugText + "%s (%" PRISIZE_T " Byte)", pFileEntry->m_path.c_str(), pFileEntry->m_size);
								}
								else
								{
									debugText.Format(debugText + "%s (%" PRISIZE_T " KiB)", pFileEntry->m_path.c_str(), pFileEntry->m_size >> 10);
								}
							}

							auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, color, false, "%s", debugText.c_str());
							posY += Debug::g_listLineHeight;
						}
					}
				}
			}
		}
	}
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
bool CFileCacheManager::FinishStreamInternal(IReadStreamPtr const pStream, int unsigned const error)
{
	bool bSuccess = false;

	auto const fileEntryId = static_cast<FileEntryId>(pStream->GetUserData());
	CFileEntry* const pFileEntry = stl::find_in_map(m_fileEntries, fileEntryId, nullptr);
	CRY_ASSERT(pFileEntry != nullptr);

	// Must be loading in to proceed.
	if ((pFileEntry != nullptr) && ((pFileEntry->m_flags & EFileFlags::Loading) != 0))
	{
		if (error == 0)
		{
			pFileEntry->m_pReadStream = nullptr;
			pFileEntry->m_flags = (pFileEntry->m_flags | EFileFlags::Cached) & ~(EFileFlags::Loading | EFileFlags::NotCached);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			pFileEntry->m_timeCached = gEnv->pTimer->GetAsyncTime().GetSeconds();
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			Impl::SFileInfo fileEntryInfo;
			fileEntryInfo.memoryBlockAlignment = pFileEntry->m_memoryBlockAlignment;
			fileEntryInfo.pFileData = pFileEntry->m_pMemoryBlock->GetData();
			fileEntryInfo.size = pFileEntry->m_size;
			fileEntryInfo.pImplData = pFileEntry->m_pImplData;
			fileEntryInfo.szFileName = PathUtil::GetFile(pFileEntry->m_path.c_str());
			fileEntryInfo.szFilePath = pFileEntry->m_path.c_str();
			fileEntryInfo.bLocalized = (pFileEntry->m_flags & EFileFlags::Localized) != 0;

			g_pIImpl->RegisterInMemoryFile(&fileEntryInfo);
			bSuccess = true;
		}
		else if (error == ERROR_USER_ABORT)
		{
			// We abort this stream only during entry Uncache().
			// Therefore there's no need to call Uncache() during stream abort with error code ERROR_USER_ABORT.

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Always, "AFCM: user aborted stream for file %s (error: %u)", pFileEntry->m_path.c_str(), error);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
		else
		{
			UncacheFileCacheEntryInternal(pFileEntry, true, true);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Error, "AFCM: failed to stream in file %s (error: %u)", pFileEntry->m_path.c_str(), error);
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CFileCacheManager::AllocateMemoryBlockInternal(CFileEntry* const __restrict pFileEntry)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CFileCacheManager::AllocateMemoryBlockInternal");

	// Must not have valid memory yet.
	CRY_ASSERT(pFileEntry->m_pMemoryBlock == nullptr);

	if (m_pMemoryHeap != nullptr)
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CFileEntry::m_pMemoryBlock");
		pFileEntry->m_pMemoryBlock = m_pMemoryHeap->AllocateBlock(pFileEntry->m_size, pFileEntry->m_path.c_str(), pFileEntry->m_memoryBlockAlignment);
	}

	if (pFileEntry->m_pMemoryBlock == nullptr)
	{
		// Memory block is either full or too fragmented, let's try to throw everything out that can be removed and allocate again.
		TryToUncacheFiles();

		// And try again!
		if (m_pMemoryHeap != nullptr)
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CFileEntry::m_pMemoryBlock");
			pFileEntry->m_pMemoryBlock = m_pMemoryHeap->AllocateBlock(pFileEntry->m_size, pFileEntry->m_path.c_str(), pFileEntry->m_memoryBlockAlignment);
		}
	}

	return pFileEntry->m_pMemoryBlock != nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::UncacheFile(CFileEntry* const pFileEntry)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	m_currentByteTotal -= pFileEntry->m_size;
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

	if (pFileEntry->m_pReadStream)
	{
		pFileEntry->m_pReadStream->Abort();
		pFileEntry->m_pReadStream = nullptr;
	}

	if (pFileEntry->m_pMemoryBlock != nullptr && pFileEntry->m_pMemoryBlock->GetData() != nullptr)
	{
		Impl::SFileInfo fileEntryInfo;
		fileEntryInfo.memoryBlockAlignment = pFileEntry->m_memoryBlockAlignment;
		fileEntryInfo.pFileData = pFileEntry->m_pMemoryBlock->GetData();
		fileEntryInfo.size = pFileEntry->m_size;
		fileEntryInfo.pImplData = pFileEntry->m_pImplData;
		fileEntryInfo.szFileName = PathUtil::GetFile(pFileEntry->m_path.c_str());

		g_pIImpl->UnregisterInMemoryFile(&fileEntryInfo);
	}

	pFileEntry->m_pMemoryBlock = nullptr;
	pFileEntry->m_flags = (pFileEntry->m_flags | EFileFlags::NotCached) & ~(EFileFlags::Cached | EFileFlags::Removable);
	CRY_ASSERT(pFileEntry->m_useCount == 0);
	pFileEntry->m_useCount = 0;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	pFileEntry->m_timeCached = 0.0f;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CFileCacheManager::TryToUncacheFiles()
{
	for (auto const& audioFileEntryPair : m_fileEntries)
	{
		CFileEntry* const pFileEntry = audioFileEntryPair.second;

		if ((pFileEntry != nullptr) &&
		    ((pFileEntry->m_flags & EFileFlags::Cached) != 0) &&
		    ((pFileEntry->m_flags & EFileFlags::Removable) != 0))
		{
			UncacheFileCacheEntryInternal(pFileEntry, true);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CFileCacheManager::UpdateLocalizedFileEntryData(CFileEntry* const pFileEntry)
{
	static Impl::SFileInfo fileEntryInfo;
	fileEntryInfo.bLocalized = true;
	fileEntryInfo.size = 0;
	fileEntryInfo.pFileData = nullptr;
	fileEntryInfo.memoryBlockAlignment = 0;

	CryFixedStringT<MaxFileNameLength> fileName(PathUtil::GetFile(pFileEntry->m_path.c_str()));
	fileEntryInfo.pImplData = pFileEntry->m_pImplData;
	fileEntryInfo.szFileName = fileName.c_str();

	pFileEntry->m_path = g_pIImpl->GetFileLocation(&fileEntryInfo);
	pFileEntry->m_path += "/";
	pFileEntry->m_path += fileName.c_str();
	pFileEntry->m_path.MakeLower();

	pFileEntry->m_size = gEnv->pCryPak->FGetSize(pFileEntry->m_path.c_str());
	CRY_ASSERT(pFileEntry->m_size > 0);
}

///////////////////////////////////////////////////////////////////////////
bool CFileCacheManager::TryCacheFileCacheEntryInternal(
	CFileEntry* const pFileEntry,
	FileEntryId const id,
	bool const bLoadSynchronously,
	bool const bOverrideUseCount /*= false*/,
	size_t const useCount /*= 0*/)
{
	bool bSuccess = false;

	if (!pFileEntry->m_path.empty() &&
	    ((pFileEntry->m_flags & EFileFlags::NotCached) != 0) &&
	    (pFileEntry->m_flags & (EFileFlags::Cached | EFileFlags::Loading)) == 0)
	{
		if (AllocateMemoryBlockInternal(pFileEntry))
		{
			StreamReadParams streamReadParams;
			streamReadParams.nOffset = 0;
			streamReadParams.nFlags = IStreamEngine::FLAGS_NO_SYNC_CALLBACK;
			streamReadParams.dwUserData = static_cast<DWORD_PTR>(id);
			streamReadParams.nLoadTime = 0;
			streamReadParams.nMaxLoadTime = 0;
			streamReadParams.ePriority = estpUrgent;
			streamReadParams.pBuffer = pFileEntry->m_pMemoryBlock->GetData();
			streamReadParams.nSize = static_cast<int unsigned>(pFileEntry->m_size);

			pFileEntry->m_flags |= EFileFlags::Loading;
			pFileEntry->m_pReadStream = gEnv->pSystem->GetStreamEngine()->StartRead(eStreamTaskTypeFSBCache, pFileEntry->m_path.c_str(), this, &streamReadParams);

			if (bLoadSynchronously)
			{
				pFileEntry->m_pReadStream->Wait();
			}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			m_currentByteTotal += pFileEntry->m_size;
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE

			bSuccess = true;
		}
		else
		{
			// Cannot have a valid memory block!
			CRY_ASSERT(pFileEntry->m_pMemoryBlock == nullptr || pFileEntry->m_pMemoryBlock->GetData() == nullptr);

			// This unfortunately is a total memory allocation fail.
			pFileEntry->m_flags |= EFileFlags::MemAllocFail;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
			// The user should be made aware of it.
			Cry::Audio::Log(ELogType::Error, R"(AFCM: could not cache "%s" as we are out of memory!)", pFileEntry->m_path.c_str());
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
		}
	}
	else if ((pFileEntry->m_flags & (EFileFlags::Cached | EFileFlags::Loading)) != 0)
	{

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		if ((pFileEntry->m_flags & EFileFlags::Loading) != 0)
		{
			Cry::Audio::Log(ELogType::Warning, R"(AFCM: could not cache "%s" as it's already loading!)", pFileEntry->m_path.c_str());
		}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

		bSuccess = true;
	}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else if ((pFileEntry->m_flags & EFileFlags::NotFound) != 0)
	{
		// The user should be made aware of it.
		Cry::Audio::Log(ELogType::Error, R"(AFCM: could not cache "%s" as it was not found at the target location!)", pFileEntry->m_path.c_str());
	}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	// Increment the used count on GameHints.
	if (((pFileEntry->m_flags & EFileFlags::UseCounted) != 0) && ((pFileEntry->m_flags & (EFileFlags::Cached | EFileFlags::Loading)) != 0))
	{
		if (bOverrideUseCount)
		{
			pFileEntry->m_useCount = useCount;
		}
		else
		{
			++pFileEntry->m_useCount;
		}

		// Make sure to handle the eAFCS_REMOVABLE flag according to the m_nUsedCount count.
		if (pFileEntry->m_useCount != 0)
		{
			pFileEntry->m_flags &= ~EFileFlags::Removable;
		}
		else
		{
			pFileEntry->m_flags |= EFileFlags::Removable;
		}
	}

	return bSuccess;
}
} // namespace CryAudio
