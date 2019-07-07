// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryMemory/MemoryAccess.h>
#include <CryThreading/IJobManager.h>
#include <CryThreading/IJobManager_JobDelegator.h>
#include <CrySystem/ISystem.h>

namespace
{
void cryAsyncMemcpy_Int(
  void* dst
  , const void* src
  , size_t size
  , int nFlags
  , volatile int* sync)
{
	cryMemcpy(dst, src, size, nFlags);
	if (sync)
		CryInterlockedDecrement(sync);
}
}

DECLARE_JOB("CryAsyncMemcpy", TCryAsyncMemcpy, cryAsyncMemcpy_Int);

#if !defined(CRY_ASYNC_MEMCPY_DELEGATE_TO_CRYSYSTEM)
CRY_ASYNC_MEMCPY_API void cryAsyncMemcpy(
#else
CRY_ASYNC_MEMCPY_API void _cryAsyncMemcpy(
#endif
  void* dst
  , const void* src
  , size_t size
  , int nFlags
  , volatile int* sync)
{
	TCryAsyncMemcpy job(dst, src, size, nFlags, sync);
	job.Run();
}
