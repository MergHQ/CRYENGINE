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

class CFileCacheManager : public IStreamCallback
{
public:

	explicit CFileCacheManager(AudioPreloadRequestLookup& rPreloadRequests);
	virtual ~CFileCacheManager();

	// Public methods
	void                Init(CryAudio::Impl::IAudioImpl* const pImpl);
	void                Release();
	void                Update();
	AudioFileEntryId    TryAddFileCacheEntry(XmlNodeRef const pFileNode, EAudioDataScope const eDataScope, bool const bAutoLoad);
	bool                TryRemoveFileCacheEntry(AudioFileEntryId const nAudioFileID, EAudioDataScope const eDataScope);
	void                UpdateLocalizedFileCacheEntries();
	void                DrawDebugInfo(IRenderAuxGeom& auxGeom, float const fPosX, float const fPosY);
	EAudioRequestStatus TryLoadRequest(AudioPreloadRequestId const nPreloadRequestID, bool const bLoadSynchronously, bool const bAutoLoadOnly);
	EAudioRequestStatus TryUnloadRequest(AudioPreloadRequestId const nPreloadRequestID);
	EAudioRequestStatus UnloadDataByScope(EAudioDataScope const eDataScope);

private:

	DELETE_DEFAULT_CONSTRUCTOR(CFileCacheManager);
	PREVENT_OBJECT_COPY(CFileCacheManager);

	// Internal type definitions.
	typedef std::map<AudioFileEntryId, CATLAudioFileEntry*, std::less<AudioFileEntryId>, STLSoundAllocator<std::pair<AudioFileEntryId, CATLAudioFileEntry*>>> TAudioFileEntries;

	// IStreamCallback
	virtual void StreamAsyncOnComplete(IReadStream* pStream, unsigned int nError);
	virtual void StreamOnComplete(IReadStream* pStream, unsigned int nError) {}
	// ~IStreamCallback

	// Internal methods
	void AllocateHeap(size_t const nSize, char const* const sUsage);
	bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, bool const bNow, bool const bIgnoreUsedCount = false);
	bool DoesRequestFitInternal(size_t const nRequestSize);
	bool FinishStreamInternal(IReadStreamPtr const pStream, int unsigned const nError);
	bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const __restrict pAudioFileEntry);
	void UncacheFile(CATLAudioFileEntry* const pAudioFileEntry);
	void TryToUncacheFiles();
	void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const pAudioFileEntry);
	bool TryCacheFileCacheEntryInternal(CATLAudioFileEntry* const pAudioFileEntry, AudioFileEntryId const nFileID, bool const bLoadSynchronously, bool const bOverrideUseCount = false, size_t const nUseCount = 0);

	// Internal members
	CryAudio::Impl::IAudioImpl*   m_pImpl;
	AudioPreloadRequestLookup&    m_rPreloadRequests;
	TAudioFileEntries             m_cAudioFileEntries;
	_smart_ptr<ICustomMemoryHeap> m_pMemoryHeap;
	size_t                        m_nCurrentByteTotal;
	size_t                        m_nMaxByteTotal;
};
