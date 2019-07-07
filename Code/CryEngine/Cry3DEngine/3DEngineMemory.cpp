// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "3DEngineMemory.h"

// Static CTemporaryPool instance
CTemporaryPool* CTemporaryPool::s_Instance = NULL;

namespace util
{
void* pool_allocate(size_t nSize)
{
	return CTemporaryPool::Get()->Allocate(nSize, 16);  // Align for possible SIMD types
}
void pool_free(void* ptr)
{
	return CTemporaryPool::Get()->Free(ptr);
}
}
