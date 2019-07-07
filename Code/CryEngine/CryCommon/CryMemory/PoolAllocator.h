// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//---------------------------------------------------------------------------
// Memory allocation class. Allocates, frees, and reuses fixed-size blocks of
// memory, a scheme sometimes known as Simple Segregated Memory.
//
// Allocation is amortized constant time. The normal case is very fast -
// basically just a couple of dereferences. If many blocks are allocated,
// the system may occasionally need to allocate a further bucket of blocks
// for itself. Deallocation is strictly fast constant time.
//
// Each PoolAllocator allocates blocks of a single size and alignment, specified
// by template arguments. There is no per-block space overhead, except for
// alignment. The free list mechanism uses the memory of the block itself
// when it is deallocated.
//
// In this implementation memory claimed by the system is never deallocated,
// until the entire allocator is deallocated. This is to ensure fast
// allocation/deallocation - reference counting the bucket quickly would
// require a pointer to the bucket be stored, whereas now no memory is used
// while the block is allocated.
//
// This allocator is suitable for use with STL lists - see STLPoolAllocator
// for an STL-compatible interface.
//
// The class can optionally support multi-threading, using the second
// template parameter. By default it is multithread-safe.
// See Synchronization.h.
//
// The class is implemented using a HeapAllocator.
//---------------------------------------------------------------------------

#pragma once

#include "HeapAllocator.h"

namespace stl
{
//! Fixed-size pool allocator, using a shared heap.
template<typename THeap>
class SharedSizePoolAllocator
{
protected:

	using_type(THeap, Lock);

	struct ObjectNode
	{
		ObjectNode* pNext;
	};

	static size_t AllocSize(size_t nSize)
	{
		return max<size_t>(nSize, sizeof(ObjectNode));
	}
	static size_t AllocAlign(size_t nSize, size_t nAlign)
	{
		if (nAlign == 0)
		{
			for (nAlign = 1; nAlign < 16; nAlign <<= 1)
			{
				if (nSize & nAlign)
					break;
			}
		}
		return nAlign;
	}

public:

	SharedSizePoolAllocator(THeap& heap, size_t nSize, size_t nAlign = 0)
		: _nAllocSize(AllocSize(nSize)),
		_nAllocAlign(AllocAlign(nSize, nAlign)),
		_pHeap(&heap),
		_pFreeList(0)
	{}

	~SharedSizePoolAllocator()
	{
		// All allocated objects should be freed by now.
		Lock lock(*_pHeap);
		Validate(lock);
		for (ObjectNode* pFree = _pFreeList; pFree; )
		{
			ObjectNode* pNext = pFree->pNext;
			_pHeap->Deallocate(lock, pFree, _nAllocSize, _nAllocAlign);
			pFree = pNext;
		}
	}

	//! Raw allocation.
	void* Allocate()
	{
		Lock lock(*_pHeap);
		if (_pFreeList)
		{
			ObjectNode* pFree = _pFreeList;
			_pFreeList = _pFreeList->pNext;
			Validate(lock);
			_Counts.nUsed++;
			return pFree;
		}

		// No free pointer, allocate a new one.
		void* pNewMemory = _pHeap->Allocate(lock, _nAllocSize, _nAllocAlign);
		if (pNewMemory)
		{
			_Counts.nUsed++;
			_Counts.nAlloc++;
			Validate(lock);
		}
		return pNewMemory;
	}

	void Deallocate(void* pObject)
	{
		Deallocate(Lock(*_pHeap), pObject);
	}

	void Deallocate(const Lock& lock, void* pObject)
	{
		if (pObject)
		{
		#ifdef _DEBUG
			// This can be slow
			assert(_pHeap->CheckPtr(lock, pObject, _nAllocSize));
		#endif

			ObjectNode* pNode = static_cast<ObjectNode*>(pObject);

			// Add the object to the front of the free list.
			pNode->pNext = _pFreeList;
			_pFreeList = pNode;
			_Counts.nUsed--;
			Validate(lock);
		}
	}

	SMemoryUsage GetCounts() const
	{
		Lock lock(*_pHeap);
		return _Counts;
	}
	SMemoryUsage GetTotalMemory(const Lock&) const
	{
		return SMemoryUsage(_Counts.nAlloc * _nAllocSize, _Counts.nUsed * _nAllocSize);
	}

	void Validate(const Lock& lock) const
	{
		_pHeap->Validate(lock);
		_Counts.Validate();
		assert(_Counts.nAlloc * _nAllocSize <= _pHeap->GetTotalMemory(lock).nUsed);
	}

	//! Only resets tracked information, does not free any data.
	//! Use when you clear the underlying heap.
	void Reset(const Lock&, bool bForce = false)
	{
		assert(bForce || _Counts.nUsed == 0);
		_Counts.Clear();
		_pFreeList = 0;
	}

protected:
	const size_t _nAllocSize, _nAllocAlign;
	SMemoryUsage _Counts;

	THeap*       _pHeap;
	ObjectNode*  _pFreeList;
};

struct SPoolMemoryUsage : SMemoryUsage
{
	size_t nPool;

	SPoolMemoryUsage(size_t _nAlloc = 0, size_t _nPool = 0, size_t _nUsed = 0)
		: SMemoryUsage(_nAlloc, _nUsed), nPool(_nPool)
	{
		Validate();
	}

	size_t nPoolFree() const
	{
		return nPool - nUsed;
	}
	size_t nNonPoolFree() const
	{
		return nAlloc - nPool;
	}
	void Validate() const
	{
		assert(nUsed <= nPool);
		assert(nPool <= nAlloc);
	}
	void Clear()
	{
		nAlloc = nUsed = nPool = 0;
	}

	void operator+=(SPoolMemoryUsage const& op)
	{
		nAlloc += op.nAlloc;
		nPool += op.nPool;
		nUsed += op.nUsed;
	}
};

//! SizePoolAllocator with owned heap.
template<typename THeap>
class SizePoolAllocator : protected THeap, public SharedSizePoolAllocator<THeap>
{
	typedef SharedSizePoolAllocator<THeap> TPool;

	using_type(THeap, Lock);
	using_type(THeap, FreeMemLock);
	using TPool::AllocSize;
	using TPool::_Counts;
	using TPool::_nAllocSize;

public:

	SizePoolAllocator(size_t nSize, size_t nAlign = 0, FHeap opts = {})
		: THeap(opts.PageSize(opts.PageSize() * AllocSize(nSize))),
		TPool(*this, nSize, nAlign)
	{
	}

	using TPool::Allocate;
	using THeap::GetMemoryUsage;

	void Deallocate(void* pObject)
	{
		FreeMemLock lock(*this);
		TPool::Deallocate(lock, pObject);
		if (THeap::FreeWhenEmpty() && _Counts.nUsed == 0)
		{
			TPool::Reset(lock);
			THeap::Clear(lock);
		}
	}

	void FreeMemoryIfEmpty()
	{
		FreeMemLock lock(*this);
		if (_Counts.nUsed == 0)
		{
			TPool::Reset(lock);
			THeap::Clear(lock);
		}
	}

	void FreeMemory()
	{
		FreeMemLock lock(*this);
		TPool::Reset(lock);
		THeap::Clear(lock);
	}

	void FreeMemoryForce()
	{
		FreeMemLock lock(*this);
		TPool::Reset(lock, true);
		THeap::Clear(lock);
	}

	SPoolMemoryUsage GetTotalMemory()
	{
		Lock lock(*this);
		return SPoolMemoryUsage(THeap::GetTotalMemory(lock).nAlloc, _Counts.nAlloc * _nAllocSize, _Counts.nUsed * _nAllocSize);
	}
};

//! Templated size version of SizePoolAllocator
template<int S, typename L = PSyncMultiThread, int A = 0>
class PoolAllocator : public SizePoolAllocator<HeapAllocator<L>>
{
public:
	PoolAllocator(FHeap opts = {})
		: SizePoolAllocator<HeapAllocator<L>>(S, A, opts)
	{
	}
};

template<int S, int A = 0>
class PoolAllocatorNoMT : public SizePoolAllocator<HeapAllocator<PSyncNone>>
{
public:
	PoolAllocatorNoMT(FHeap opts = {})
		: SizePoolAllocator<HeapAllocator<PSyncNone>>(S, A, opts)
	{
	}
};

template<typename T, typename L = PSyncMultiThread, size_t A = 0>
class TPoolAllocator : public SizePoolAllocator<HeapAllocator<L>>
{
	typedef SizePoolAllocator<HeapAllocator<L>> TSizePool;

public:

	using TSizePool::Allocate;
	using TSizePool::Deallocate;

	TPoolAllocator(FHeap opts = {})
		: TSizePool(sizeof(T), max<size_t>(alignof(T), A), opts)
	{}

	T* New()
	{
		return ::new(Allocate())T();
	}

	template<class I>
	T* New(const I& init)
	{
		return ::new(Allocate())T(init);
	}

	void Delete(T* ptr)
	{
		if (ptr)
		{
			ptr->~T();
			Deallocate(ptr);
		}
	}
};

typedef PSyncNone        PoolAllocatorSynchronizationSinglethreaded;    //!< Legacy verbose typedef.
typedef PSyncMultiThread PoolAllocatorSynchronizationMultithreaded;     //!< Legacy verbose typedef.

};