// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "PoolAllocator.h"

#include "Pool.h"

namespace Memory
{

void* PoolAllocator::Allocate(uint32 length)
{
	CRY_ASSERT(m_pPool);
	return m_pPool->Allocate(length);
}

void PoolAllocator::Free(void* pAddress)
{
	CRY_ASSERT(m_pPool);
	m_pPool->Free(pAddress);
}

size_t PoolAllocator::GetGuaranteedAlignment() const
{
	CRY_ASSERT(m_pPool);
	return m_pPool->GetGuaranteedAlignment();
}

std::unique_ptr<IAllocator> PoolAllocator::Clone() const
{
	return std::unique_ptr<IAllocator>(new PoolAllocator(*this));
}

} // namespace Memory
