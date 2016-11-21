// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMemory/CryMemoryManager.h>

#ifdef USE_GLOBAL_BUCKET_ALLOCATOR
	#include <CryMemory/BucketAllocatorImpl.h>
#endif // USE_GLOBAL_BUCKET_ALLOCATOR

#include <CrySystem/Profilers/FrameProfiler/FrameProfiler.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

#if !defined(_RELEASE)
	#define INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
#endif // _RELEASE

class IGeneralMemoryHeap;

// These macros can be used to allocate memory in the module's memory pool.
// The first one allows for allocating memory of custom size.
// The second one creates a PTR that stores a new instance of a CLASS, the third one uses a pre-created PTR to save the new instance.
// Second and third will both only execute the constructor if the allocation succeeded.
#define POOL_NEW_CUSTOM(CLASS, PTR, NUM) PTR = static_cast<CLASS*>(AUDIO_ALLOCATOR_MEMORY_POOL.Allocate<void*>(sizeof(CLASS) * static_cast<size_t>(NUM), AUDIO_MEMORY_ALIGNMENT, # CLASS));
#define POOL_NEW_CREATE(CLASS, PTR)      CLASS * PTR = static_cast<CLASS*>(AUDIO_ALLOCATOR_MEMORY_POOL.Allocate<void*>(sizeof(CLASS), AUDIO_MEMORY_ALIGNMENT, # CLASS)); (PTR == nullptr) ? nullptr : new(PTR) CLASS
#define POOL_NEW(CLASS, PTR)             PTR = static_cast<CLASS*>(AUDIO_ALLOCATOR_MEMORY_POOL.Allocate<void*>(sizeof(CLASS), AUDIO_MEMORY_ALIGNMENT, # CLASS)); (PTR == nullptr) ? nullptr : new(PTR) CLASS

// These two macros are used to destroy the objects allocated in the module's memory pool and free their memory
// The second one is designed to be used for const pointers
#define POOL_FREE(PTR)       { stl::destruct(PTR); bool const bFreed = AUDIO_ALLOCATOR_MEMORY_POOL.Free(PTR); CRY_ASSERT(bFreed); }
#define POOL_FREE_CONST(PTR) { stl::destruct(PTR); bool const bFreed = AUDIO_ALLOCATOR_MEMORY_POOL.Free((void*) PTR); CRY_ASSERT(bFreed); }

/**
 * A wrapper class for all pool allocations. Small allocations are redirected to the global bucket allocator,
 * the larger ones are kept in the module's memory pool.
 */
template<size_t TBucketSize>
class CSoundAllocator
{
public:

	//DOC-IGNORE-BEGIN
	CSoundAllocator()
		: m_mspaceBase(nullptr)
		, m_mspaceSize(0)
		, m_mspace(nullptr)
		, m_mspaceUsed(0)
		, nSmallAllocations(0)
		, nAllocations(0)
		, nSmallAllocationsSize(0)
		, nAllocationsSize(0)
	{
#if CAPTURE_REPLAY_LOG && defined(USE_GLOBAL_BUCKET_ALLOCATOR)
		m_smallAllocator.ReplayRegisterAddressRange("Sound Buckets");
#endif
	}

	~CSoundAllocator()
	{
		if (m_mspace)
			m_mspace->Release();
	}

	CSoundAllocator(CSoundAllocator const&) = delete;
	CSoundAllocator& operator=(CSoundAllocator const&) = delete;
	//DOC-IGNORE-END

	void* AllocateRaw(size_t const nSize, size_t const nAlign, char const* const sUsage)
	{
		void* pTemp = nullptr;

		// Redirect small allocations through a bucket allocator
		if (nSize <= nMaxSmallSize)
		{
			pTemp = m_smallAllocator.allocate(nSize);
			TrackSmallAlloc(pTemp, m_smallAllocator.getSizeEx(pTemp));
		}
		else
		{
			FRAME_PROFILER("CSoundAllocator::Allocate", GetISystem(), PROFILE_AUDIO);

			pTemp = m_mspace->Memalign(nAlign, nSize, sUsage);

			if (pTemp != nullptr)
			{
				CryInterlockedAdd(&m_mspaceUsed, m_mspace->UsableSize(pTemp));
			}

			TrackGeneralAlloc(pTemp, nSize);
		}

#if defined(INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE)
		if ((reinterpret_cast<uintptr_t>(pTemp) & (nAlign - 1)) > 0)
		{
			CryFatalError("<Audio>: allocation not %" PRISIZE_T "byte aligned!", nAlign);
		}
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE

		return pTemp;
	}

	template<typename T>
	T Allocate(size_t const nSize, size_t const nAlign, char const* const sUsage)
	{
		return reinterpret_cast<T>(AllocateRaw(nSize, nAlign, sUsage));
	}

	bool Free(void* ptr)
	{
		if (ptr && m_smallAllocator.IsInAddressRange(ptr))
		{
			m_smallAllocator.deallocate(ptr);
			TrackSmallDealloc(ptr);

			return true;
		}
		else
		{
			size_t sz = m_mspace->Free(ptr);
			if (sz)
			{
				TrackGeneralDealloc(sz);
				CryInterlockedAdd(&m_mspaceUsed, -static_cast<int>(sz));

				return true;
			}
		}

		return false;
	}

	size_t Size(void* ptr)
	{
		if (m_smallAllocator.IsInAddressRange(ptr))
		{
			return m_smallAllocator.getSizeEx(ptr);
		}
		else
		{
			return m_mspace->UsableSize(ptr);
		}
	}

	ILINE uint8*       Data() const                 { return reinterpret_cast<uint8*>(m_mspaceBase); }
	ILINE size_t       MemSize() const              { return m_mspaceSize; }
	ILINE size_t       MemFree() const              { return m_mspaceSize - static_cast<size_t>(m_mspaceUsed); }
	ILINE size_t const FragmentCount()        const { return static_cast<size_t>(nAllocations); }
	ILINE size_t const GetSmallAllocsSize()   const { return static_cast<size_t>(nSmallAllocationsSize); }
	ILINE size_t const GetSmallAllocsCount()  const { return static_cast<size_t>(nSmallAllocations); }

	void               InitMem(size_t const nSize, void* ptr, char const* const sDescription)
	{
		m_mspace = CryGetIMemoryManager()->CreateGeneralMemoryHeap(ptr, nSize, sDescription);
		m_mspaceBase = ptr;
		m_mspaceSize = nSize;
	}

	void UnInitMem()
	{
		if (m_mspace)
			m_mspace->Release();

		m_mspaceBase = nullptr;
		m_mspaceSize = 0;
		m_mspace = nullptr;
		m_mspaceUsed = 0;
		nSmallAllocations = 0;
		nAllocations = 0;
		nSmallAllocationsSize = 0;
		nAllocationsSize = 0;
	}

	void Cleanup()
	{
#if defined(USE_GLOBAL_BUCKET_ALLOCATOR)
		m_smallAllocator.cleanup();
#endif
	}

private:

#ifdef USE_GLOBAL_BUCKET_ALLOCATOR
	typedef BucketAllocator<BucketAllocatorDetail::DefaultTraits<TBucketSize, BucketAllocatorDetail::SyncPolicyLocked, false>> SoundBuckets;
#else
	typedef node_alloc<eCryDefaultMalloc, true, 512* 1024>                                                                     SoundBuckets;
#endif

	void TrackSmallAlloc(void* ptr, size_t nSize)
	{
#if defined(INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE)
		if (ptr)
		{
			CryInterlockedIncrement(&nSmallAllocations);
			CryInterlockedAdd(&nSmallAllocationsSize, nSize);
		}
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
	}

	void TrackGeneralAlloc(void* ptr, size_t nSize)
	{
#if defined(INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE)
		if (ptr)
		{
			CryInterlockedIncrement(&nAllocations);
			CryInterlockedAdd(&nAllocationsSize, nSize);
		}
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
	}

	void TrackSmallDealloc(void* ptr)
	{
#if defined(INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE)
		CryInterlockedDecrement(&nSmallAllocations);
		CryInterlockedAdd(&nSmallAllocationsSize, -static_cast<int>(Size(ptr)));
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
	}

	void TrackGeneralDealloc(size_t sz)
	{
#if defined(INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE)
		CryInterlockedDecrement(&nAllocations);
		CryInterlockedAdd(&nAllocationsSize, -static_cast<int>(sz));
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
	}

	// all allocations below nMaxSmallSize go into the system-wide allocator(and thus the system wide small allocation allocator)
	enum {nMaxSmallSize = 512};

	void*               m_mspaceBase;
	size_t              m_mspaceSize;
	IGeneralMemoryHeap* m_mspace;
	volatile int        m_mspaceUsed;

	SoundBuckets        m_smallAllocator;

	volatile int        nSmallAllocations;
	volatile int        nAllocations;

	volatile int        nSmallAllocationsSize;
	volatile int        nAllocationsSize;
};
