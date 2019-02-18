// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/IStreamEngine.h>
#include <CryMemory/IMemory.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
struct IRenderAuxGeom;
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
struct ICustomMemoryHeap;
class CFileEntry;

class CFileCacheManager final : public IStreamCallback
{
public:

	CFileCacheManager() = default;
	~CFileCacheManager();

	CFileCacheManager(CFileCacheManager const&) = delete;
	CFileCacheManager(CFileCacheManager&&) = delete;
	CFileCacheManager& operator=(CFileCacheManager const&) = delete;
	CFileCacheManager& operator=(CFileCacheManager&&) = delete;

	// Public methods
	void           Initialize();
	FileEntryId    TryAddFileCacheEntry(XmlNodeRef const pFileNode, EDataScope const dataScope, bool const bAutoLoad);
	bool           TryRemoveFileCacheEntry(FileEntryId const id, EDataScope const dataScope);
	void           UpdateLocalizedFileCacheEntries();
	ERequestStatus TryLoadRequest(PreloadRequestId const preloadRequestId, bool const bLoadSynchronously, bool const bAutoLoadOnly);
	ERequestStatus TryUnloadRequest(PreloadRequestId const preloadRequestId);
	ERequestStatus UnloadDataByScope(EDataScope const dataScope);

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	void DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY);
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

private:

	// Internal type definitions.
	using FileEntries = std::map<FileEntryId, CFileEntry*>;

	// IStreamCallback
	virtual void StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError) override;
	virtual void StreamOnComplete(IReadStream* pStream, unsigned int nError) override {}
	// ~IStreamCallback

	// Internal methods
	bool UncacheFileCacheEntryInternal(CFileEntry* const pFileEntry, bool const bNow, bool const bIgnoreUsedCount = false);
	bool FinishStreamInternal(IReadStreamPtr const pStream, int unsigned const error);
	bool AllocateMemoryBlockInternal(CFileEntry* const __restrict pFileEntry);
	void UncacheFile(CFileEntry* const pFileEntry);
	void TryToUncacheFiles();
	void UpdateLocalizedFileEntryData(CFileEntry* const pFileEntry);
	bool TryCacheFileCacheEntryInternal(CFileEntry* const pFileEntry, FileEntryId const id, bool const bLoadSynchronously, bool const bOverrideUseCount = false, size_t const useCount = 0);

	// Internal members
	FileEntries                     m_fileEntries;
	_smart_ptr<::ICustomMemoryHeap> m_pMemoryHeap;

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	size_t m_currentByteTotal = 0;

	#if CRY_PLATFORM_DURANGO
	size_t m_maxByteTotal = 0;
	#endif // CRY_PLATFORM_DURANGO
#endif   // CRY_AUDIO_USE_PRODUCTION_CODE
};
} // namespace CryAudio
