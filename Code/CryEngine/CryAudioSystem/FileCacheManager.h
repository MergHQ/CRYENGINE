// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ATLEntities.h"
#include <CrySystem/IStreamEngine.h>

// Forward declarations
struct ICustomMemoryHeap;
class CATLAudioFileEntry;
struct IRenderAuxGeom;

// Filter for drawing debug info to the screen
enum EAudioFileCacheManagerDebugFilter
{
	eAFCMDF_ALL             = 0,
	eAFCMDF_GLOBALS         = BIT(6), // a
	eAFCMDF_LEVEL_SPECIFICS = BIT(7), // b
	eAFCMDF_USE_COUNTED     = BIT(8), // c
};

class CFileCacheManager final : public IStreamCallback
{
public:

	explicit CFileCacheManager(AudioPreloadRequestLookup& preloadRequests);

	CFileCacheManager(CFileCacheManager const&) = delete;
	CFileCacheManager(CFileCacheManager&&) = delete;
	CFileCacheManager& operator=(CFileCacheManager const&) = delete;
	CFileCacheManager& operator=(CFileCacheManager&&) = delete;

	// Public methods
	void                Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                Release();
	void                Update();
	AudioFileEntryId    TryAddFileCacheEntry(XmlNodeRef const pFileNode, EAudioDataScope const dataScope, bool const bAutoLoad);
	bool                TryRemoveFileCacheEntry(AudioFileEntryId const audioFileEntryId, EAudioDataScope const dataScope);
	void                UpdateLocalizedFileCacheEntries();
	void                DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float const posY);
	EAudioRequestStatus TryLoadRequest(AudioPreloadRequestId const audioPreloadRequestId, bool const bLoadSynchronously, bool const bAutoLoadOnly);
	EAudioRequestStatus TryUnloadRequest(AudioPreloadRequestId const audioPreloadRequestId);
	EAudioRequestStatus UnloadDataByScope(EAudioDataScope const dataScope);

private:

	// Internal type definitions.
	typedef std::map<AudioFileEntryId, CATLAudioFileEntry*, std::less<AudioFileEntryId>, STLSoundAllocator<std::pair<AudioFileEntryId, CATLAudioFileEntry*>>> AudioFileEntries;

	// IStreamCallback
	virtual void StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError) override;
	virtual void StreamOnComplete(IReadStream* pStream, unsigned int nError) override {}
	// ~IStreamCallback

	// Internal methods
	void AllocateHeap(size_t const size, char const* const szUsage);
	bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, bool const bNow, bool const bIgnoreUsedCount = false);
	bool DoesRequestFitInternal(size_t const requestSize);
	bool FinishStreamInternal(IReadStreamPtr const pStream, int unsigned const error);
	bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict pAudioFileEntry);
	void UncacheFile(CATLAudioFileEntry* const pAudioFileEntry);
	void TryToUncacheFiles();
	void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const pAudioFileEntry);
	bool TryCacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, AudioFileEntryId const audioFileEntryId, bool const bLoadSynchronously, bool const bOverrideUseCount = false, size_t const useCount = 0);

	// Internal members
	CryAudio::Impl::IAudioImpl*   m_pImpl;
	AudioPreloadRequestLookup&    m_preloadRequests;
	AudioFileEntries              m_audioFileEntries;
	_smart_ptr<ICustomMemoryHeap> m_pMemoryHeap;
	size_t                        m_currentByteTotal;
	size_t                        m_maxByteTotal;
};
