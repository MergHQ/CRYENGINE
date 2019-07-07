// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryThreading/CryThread.h>

namespace NCryPoolAlloc
{

template<class TAllocator>
class CThreadSafe : public TAllocator
{
	CryCriticalSection m_Mutex;
public:

	template<class T>
	ILINE T Allocate(size_t Size, size_t Align = 1)
	{
		CryAutoLock<CryCriticalSection> lock(m_Mutex);
		return TAllocator::template Allocate<T>(Size, Align);
	}

	template<class T>
	ILINE bool Free(T pData, bool ForceBoundsCheck = false)
	{
		CryAutoLock<CryCriticalSection> lock(m_Mutex);
		return TAllocator::Free(pData, ForceBoundsCheck);
	}

	template<class T>
	ILINE bool Resize(T** pData, size_t Size, size_t Alignment)
	{
		CryAutoLock<CryCriticalSection> lock(m_Mutex);
		return TAllocator::Resize(pData, Size, Alignment);
	}

};

}
