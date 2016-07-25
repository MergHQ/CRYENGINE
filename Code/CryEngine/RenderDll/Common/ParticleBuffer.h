// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <array>
#include <CryRenderer/RenderElements/CREParticle.h>

class CParticleSubBuffer
{
public:
	CParticleSubBuffer();
	~CParticleSubBuffer();

	void    Create(uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags);
	void    Release();
	byte*   Lock();
	void    Unlock(uint32 size);

	HRESULT BindVB(uint streamNumber = 0, int bytesOffset = 0, int stride = 0);
	HRESULT BindIB(uint offset);
	void    BindSRV(CDeviceManager::SHADER_TYPE shaderType, uint slot);

	uint    GetStride() const { return m_stride; }

private:
	CGpuBuffer m_buffer;
	byte*      m_pLockedData;
	uint       m_elemCount;
	uint       m_stride;
	uint       m_flags;
};

class CParticleBufferSet
{
private:
	typedef std::array<CParticleSubBuffer, CREParticle::numBuffers> TDynBuffer;
	typedef std::array<DeviceFenceHandle, CREParticle::numBuffers>  TDeviceFences;

	struct SubBuffer
	{
		TDynBuffer m_buffers;
		// During writing, we only expose the base Video Memory pointer
		// and the offsets to the next free memory in this buffer
		byte*  m_pMemoryBase[CREParticle::numBuffers];
		uint32 m_offset[CREParticle::numBuffers];
		uint32 m_availableMemory;
	};

public:
	enum EBufferTypes
	{
		EBT_Vertices,
		EBT_Indices,
		EBT_PositionsSRV,
		EBT_AxesSRV,
		EBT_ColorSTsSRV,
		EBT_Total,
	};

	struct SAlloc
	{
		byte* m_pBase;
		uint  m_firstElem;
		uint  m_numElemns;
	};

	struct SAllocStreams
	{
		Vec3*             m_pPositions;
		SParticleAxes*    m_pAxes;
		SParticleColorST* m_pColorSTs;
		uint              m_firstElem;
		uint              m_numElemns;
	};

public:
	CParticleBufferSet();
	~CParticleBufferSet();

	void Create(uint poolSize);
	void Release();
	uint GetMaxNumSprites() const { return m_maxSpriteCount; }

	void Lock();
	void Unlock();
	void SetFence();
	void WaitForFence();
	bool IsValid() const;

	uint GetAllocId() const;
	uint GetBindId() const;
	void Alloc(uint index, EBufferTypes type, uint numElems, SAlloc* pAllocOut);
	void Alloc(uint index, uint numElems, SAllocStreams* pAllocOut);

	void BindVB();
	void BindIB();
	void BindSRVs();
	void BindSpriteIB();

private:
	void CreateSubBuffer(EBufferTypes type, uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags);
	void CreateSpriteBuffer(uint poolSize);

	std::array<SubBuffer, EBT_Total> m_subBuffers;
	TDeviceFences                    m_fences;
	CGpuBuffer                       m_spriteIndexBuffer;
	uint                             m_maxSpriteCount;
	uint                             m_ids[RT_COMMAND_BUF_COUNT];
	bool                             m_valid;
};
