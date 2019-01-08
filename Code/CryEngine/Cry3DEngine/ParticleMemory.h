// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryMemory/PoolAllocator.h"

struct ParticleHeap: stl::HeapAllocator<stl::PSyncMultiThread>
{
	ParticleHeap()
		: HeapAllocator<stl::PSyncMultiThread>(FHeap().PageSize(0x10000).FreeWhenEmpty(true))
	{}
};

typedef stl::StaticPoolCommonAllocator<ParticleHeap> ParticleAllocator;

