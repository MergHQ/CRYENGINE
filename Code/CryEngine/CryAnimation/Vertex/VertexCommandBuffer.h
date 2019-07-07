// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "VertexData.h"
#include <CryThreading/IJobManager.h>

class CVertexCommandBufferAllocator
{
public:
	virtual ~CVertexCommandBufferAllocator() {}

public:
	virtual void* Allocate(const uint length) = 0;
};

//

class CVertexCommandBufferAllocationCounter :
	public CVertexCommandBufferAllocator
{
public:
	CVertexCommandBufferAllocationCounter();
	virtual ~CVertexCommandBufferAllocationCounter();

public:
	ILINE uint GetCount() const { return m_count; }

	// CVertexCommandBufferAllocator
public:
	virtual void* Allocate(const uint length);

private:
	uint m_count;
};

//

class CVertexCommandBufferAllocatorStatic :
	public CVertexCommandBufferAllocator
{
public:
	CVertexCommandBufferAllocatorStatic(const void* pMemory, const uint length);
	~CVertexCommandBufferAllocatorStatic();

	// CVertexCommandBufferAllocator
public:
	virtual void* Allocate(const uint length);

private:
	uint8* m_pMemory;
	uint   m_memoryLeft;
};

//

class CVertexCommandBuffer
{
public:
	bool Initialize(CVertexCommandBufferAllocator& allocator)
	{
		m_pAllocator = &allocator;
		m_pCommands = NULL;
		m_commandsLength = 0;
		return true;
	}

	template<class Type>
	Type* AddCommand();

	void  Process(CVertexData& vertexData);

private:
	CVertexCommandBufferAllocator* m_pAllocator;
	const uint8*                   m_pCommands;
	uint                           m_commandsLength;
};

//

template<class Type>
Type* CVertexCommandBuffer::AddCommand()
{
	uint length = sizeof(Type);
	void* pAllocation = m_pAllocator->Allocate(length);
	if (!pAllocation)
		return NULL;

	if (!m_pCommands)
		m_pCommands = (const uint8*)pAllocation;

	m_commandsLength += length;

	Type* pCommand = (Type*)pAllocation;
	new(pCommand) Type();
	pCommand->length = length;
	return pCommand;
}

struct SVertexAnimationJob
{
	CVertexData          vertexData;
	CVertexCommandBuffer commandBuffer;
	uint                 commandBufferLength;

	IRenderMesh*         m_pRenderMesh;
	IRenderMesh*         m_pPreviousRenderMesh;

	volatile int*        pRenderMeshSyncVariable;

	SVertexAnimationJob()
		: vertexData()
		, commandBuffer()
		, commandBufferLength()
		, m_pRenderMesh()
		, m_pPreviousRenderMesh()
		, pRenderMeshSyncVariable()
	{}

	void Begin(JobManager::SJobState* pJob);
	void Execute(int);
};
