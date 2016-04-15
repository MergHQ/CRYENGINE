/*
	SH allocator implementations
*/

#include "stdafx.h"
#include <PRT/SHAllocator.h>


SH_ALLOCATOR_API void *SHMalloc(size_t Size)
{
	return malloc(Size);
}

SH_ALLOCATOR_API void SHFreeSize(void *pPtr, size_t)
{
	free(pPtr);
}