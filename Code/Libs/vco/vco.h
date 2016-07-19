#pragma once
#include <stdint.h>

extern "C"
{
	// Optimizes an index buffer in-place for improved cache performance.
	// The function may or may not do something, based on SDK availability.
	// The indices must form a triangle-list, and should already have been de-duplicated if possible.
	void OptimizeIndexBufferForOrbis(uint32_t* pIndices, uint32_t numIndices);
}
