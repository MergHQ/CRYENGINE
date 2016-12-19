// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "PoolObject.h"

#if !defined(_RELEASE)
	#define INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
#endif // _RELEASE

#ifdef INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE
	#include <CryAudio/IAudioSystem.h>
	#include <CrySystem/ISystem.h>
#endif // INCLUDE_AUDIO_ALLOCATOR_PRODUCTION_CODE

template<typename T>
typename CPoolObject<T>::Allocator * CPoolObject<T>::ms_pAllocator = nullptr;

//////////////////////////////////////////////////////////////////////////
template<typename T>
typename CPoolObject<T>::Allocator & CPoolObject<T>::GetAllocator() noexcept
{
	//return m_allocator;
	CRY_ASSERT_MESSAGE(ms_pAllocator, "Trying to get allocator before calling CreateAllocator()");
	return *ms_pAllocator;
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void CPoolObject<T >::CreateAllocator(uint16 const preallocatedObjects)
{
	CRY_ASSERT_MESSAGE(ms_pAllocator == nullptr, "Trying to re-create the pool object allocator");
	CRY_ASSERT_MESSAGE(preallocatedObjects > 0, "Trying to create a pool object allocator with zero objects");
	static CPoolObject<T>::Allocator allocator(sizeof(T), alignof(T), stl::FHeap().PageSize(preallocatedObjects).FreeWhenEmpty(true));
	ms_pAllocator = &allocator;
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void* CPoolObject<T >::AllocateObjectStorage()
{
	// Poor man's replacement for lack of a proper __attribute__((malloc))/
	// restrict function attribute.
	auto* __restrict const pMemory(GetAllocator().Allocate());
	return pMemory;
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void* CPoolObject<T >::operator new(std::size_t const count, std::nothrow_t const&) noexcept
{
	CRY_ASSERT_MESSAGE(count % sizeof(T) == 0, "Allocating a partial object!?");
	CRY_ASSERT_MESSAGE(count / sizeof(T) == 1, "Allocating more than one object not supported with pool allocators.");
	auto* __restrict const pMemory(AllocateObjectStorage());
	return pMemory;
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void* CPoolObject<T >::operator new(std::size_t const count)
{
	auto* __restrict const pMemory(CPoolObject<T>::operator new(count, std::nothrow));
	return pMemory;
}

//////////////////////////////////////////////////////////////////////////
template<typename T> NO_INLINE
void CPoolObject<T >::operator delete(void* const pObject) noexcept
{
	GetAllocator().Deallocate(pObject);
}
