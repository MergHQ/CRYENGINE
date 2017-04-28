// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   DeviceManager.cpp : Device manager

   Revision history:
* Created by Khonich Andrey

   =============================================================================*/

#include "StdAfx.h"
#include "../DriverD3D.h"

#include <CryThreading/IJobManager.h>
#include <CryThreading/IJobManager_JobDelegator.h>

#include "DeviceManager.h"

#if CRY_PLATFORM_ORBIS
	#include "GNM/DeviceManager_GNM.inl"
#elif defined(CRY_USE_DX12)
	#include "D3D12/DeviceManager_D3D12.inl"
#else
	#include "D3D11/DeviceManager_D3D11.inl"

	#if CRY_PLATFORM_DURANGO
		#include "D3D11/DeviceManager_D3D11_Durango.h"
		#include "D3D11/DeviceManager_D3D11_Durango.inl"
	#endif

	#if defined(USE_NV_API)
		#include "D3D11/DeviceManager_D3D11_NVAPI.h"
		#include "D3D11/DeviceManager_D3D11_NVAPI.inl"
	#endif
#endif

CDeviceManager::CDeviceManager()
	: m_fence_handle(0)
#if DURANGO_ENABLE_ASYNC_DIPS == 1
	#if CRY_PLATFORM_DURANGO
	, m_nPush(0)
	, m_nPull(0)
	#endif
	, m_nAsyncDipJobCounter(0)
#endif // DURANGO_ENABLE_ASYNC_DIPS == 1
{
#if CRY_PLATFORM_DURANGO && DURANGO_ENABLE_ASYNC_DIPS == 1
	memset(m_arrParams, 0, sizeof(m_arrParams));
#endif
	memset(m_NullResources, 0, sizeof(m_NullResources));
}

CDeviceManager::~CDeviceManager()
{
#if DURANGO_ENABLE_ASYNC_DIPS
	CryAlignedDelete(s_dip_queue);
#endif
	if (m_fence_handle != DeviceFenceHandle() && FAILED(ReleaseFence(m_fence_handle)))
	{
		CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING,
		           "could not create sync fence");
	}

	for (enum D3D11_RESOURCE_DIMENSION eType = D3D11_RESOURCE_DIMENSION_UNKNOWN; eType <= D3D11_RESOURCE_DIMENSION_TEXTURE3D; eType = D3D11_RESOURCE_DIMENSION(eType + 1))
	{
		if (auto pNullResource = m_NullResources[eType])
		{
			ULONG refCount = pNullResource->Release();
			assert(refCount == 0);
		}
	}
}

void CDeviceManager::Init()
{
#if CRY_PLATFORM_DURANGO
	UINT poolMemModel =
	#if defined(TEXTURES_IN_CACHED_MEM)
	  D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_NONCOHERENT_GPU_READONLY
	#else
	  D3D11_GRAPHICS_MEMORY_ACCESS_CPU_WRITECOMBINE_NONCOHERENT
	#endif
	;

	m_numInvalidDrawcalls = 0;

	m_texturePool.Init(
		CRenderer::GetTexturesStreamPoolSize() * 1024 * 1024,
		512 * 1024 * 1024,
		CRenderer::GetTexturesStreamPoolSize() * 1024 * 1024,
		poolMemModel,
		false);
	m_textureStagingRing.Init(gcpRendD3D->m_pDMA1, 128 * 1024 * 1024);
#endif

#if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	memset(m_CB, 0x0, sizeof(m_CB));
	memset(m_SRV, 0x0, sizeof(m_SRV));
	memset(m_UAV, 0x0, sizeof(m_UAV));
	memset(m_Samplers, 0x0, sizeof(m_Samplers));
	memset(&m_VBs, 0x0, sizeof(m_VBs));
	memset(&m_IB, 0x0, sizeof(m_IB));
	memset(&m_VertexDecl, 0x0, sizeof(m_VertexDecl));
	memset(&m_Topology, 0x0, sizeof(m_Topology));
	memset(&m_Shaders, 0x0, sizeof(m_Shaders));
	memset(&m_RasterState, 0x0, sizeof(m_RasterState));
	memset(&m_DepthStencilState, 0x0, sizeof(m_DepthStencilState));
	memset(&m_BlendState, 0x0, sizeof(m_BlendState));
#endif

#if DURANGO_ENABLE_ASYNC_DIPS
	CryAlignedDelete(s_dip_queue);
	s_dip_queue = CryAlignedNew<TAsyncDIPQueue>();
#endif
}

void CDeviceManager::RT_Tick()
{
	m_numInvalidDrawcalls = 0;

#if CRY_PLATFORM_DURANGO
	m_texturePool.RT_Tick();
#endif
}

//=============================================================================

ILINE bool CDeviceManager::ValidateDrawcall()
{
	if (CHWShader_D3D::s_nActivationFailMask != 0)
	{
		++m_numInvalidDrawcalls;
		return false;
	}

	return true;
}

void CDeviceManager::Draw(uint32 nVerticesCount, uint32 nStartVertex)
{
	if (!ValidateDrawcall())
	{
		return;
	}

#if DURANGO_ENABLE_ASYNC_DIPS
	if (CRenderer::CV_r_durango_async_dips)
	{
		if (s_CommandBuffer.m_current_offset * 4 >= SCommandBuffer::COMMAND_BUFFER_SIZE * 3)
		{
			gcpRendD3D.WaitForAsynchronousDevice();
			s_CommandBuffer.m_current_offset = 0;
		}

		char* start = &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset];
		CommitDeviceStatesAsync();

		SDraw* pJob = new(s_CommandBuffer.Allocate(sizeof(SDraw)))SDraw;
		pJob->nVerticesCount = nVerticesCount;
		pJob->nStartVertex = nStartVertex;

	#if CRY_PLATFORM_DURANGO
		SubmitAsyncDipJob(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
	#else
		TExecuteAsyncDIP dip;
		TExecuteAsyncDIPPacket packet(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
		s_dip_queue->AddPacket(packet, JobManager::eHighPriority, &dip, false);
	#endif // CRY_PLATFORM_DURANGO

		++m_nAsyncDipJobCounter;
		if (m_nAsyncDipJobCounter >= CRenderer::CV_r_durango_async_dips_sync)
		{
			gcpRendD3D->WaitForAsynchronousDevice();
			m_nAsyncDipJobCounter = 0;
		}

		return;
	}
#endif

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext_Unsynchronized().Draw(nVerticesCount, nStartVertex);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::DrawInstanced(uint32 nInstanceVerts, uint32 nInstances, uint32 nStartVertex, uint32 nStartInstance)
{
	if (!ValidateDrawcall())
	{
		return;
	}

#if DURANGO_ENABLE_ASYNC_DIPS
	if (CRenderer::CV_r_durango_async_dips)
	{
		if (s_CommandBuffer.m_current_offset * 4 >= SCommandBuffer::COMMAND_BUFFER_SIZE * 3)
		{
			gcpRendD3D.WaitForAsynchronousDevice();
			s_CommandBuffer.m_current_offset = 0;
		}

		char* start = &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset];
		CommitDeviceStatesAsync();

		SDrawInstanced* pJob = new(s_CommandBuffer.Allocate(sizeof(SDrawInstanced)))SDrawInstanced;
		pJob->nInstanceVerts = nInstanceVerts;
		pJob->nInstanceCount = nInstances;
		pJob->nStartVertex = nStartVertex;
		pJob->nStartInstance = nStartInstance;

	#if CRY_PLATFORM_DURANGO
		SubmitAsyncDipJob(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
	#else
		TExecuteAsyncDIP dip;
		TExecuteAsyncDIPPacket packet(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
		s_dip_queue->AddPacket(packet, JobManager::eHighPriority, &dip, false);
	#endif // CRY_PLATFORM_DURANGO

		++m_nAsyncDipJobCounter;
		if (m_nAsyncDipJobCounter >= CRenderer::CV_r_durango_async_dips_sync)
		{
			gcpRendD3D->WaitForAsynchronousDevice();
			m_nAsyncDipJobCounter = 0;
		}

		return;
	}
#endif

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext_Unsynchronized().DrawInstanced(nInstanceVerts, nInstances, nStartVertex, nStartInstance);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::DrawIndexedInstanced(uint32 numIndices, uint32 nInsts, uint32 startIndex, uint32 v0, uint32 v1)
{
	if (!ValidateDrawcall())
	{
		return;
	}

#if DURANGO_ENABLE_ASYNC_DIPS
	if (CRenderer::CV_r_durango_async_dips)
	{
		if (s_CommandBuffer.m_current_offset * 4 >= SCommandBuffer::COMMAND_BUFFER_SIZE * 3)
		{
			gcpRendD3D.WaitForAsynchronousDevice();
			s_CommandBuffer.m_current_offset = 0;
		}

		char* start = &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset];
		CommitDeviceStatesAsync();

		SDrawIndexedInstanced* pJob = new(s_CommandBuffer.Allocate(sizeof(SDrawIndexedInstanced)))SDrawIndexedInstanced;
		pJob->numIndices = numIndices;
		pJob->nInsts = nInsts;
		pJob->startIndex = startIndex;
		pJob->v0 = v0;
		pJob->v1 = v1;

	#if CRY_PLATFORM_DURANGO
		SubmitAsyncDipJob(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
	#else
		TExecuteAsyncDIP dip;
		TExecuteAsyncDIPPacket packet(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
		s_dip_queue->AddPacket(packet, JobManager::eHighPriority, &dip, false);
	#endif // CRY_PLATFORM_DURANGO

		++m_nAsyncDipJobCounter;
		if (m_nAsyncDipJobCounter >= CRenderer::CV_r_durango_async_dips_sync)
		{
			gcpRendD3D->WaitForAsynchronousDevice();
			m_nAsyncDipJobCounter = 0;
		}

		return;
	}
#endif

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext_Unsynchronized().DrawIndexedInstanced(numIndices, nInsts, startIndex, v0, v1);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::DrawIndexed(uint32 numIndices, uint32 startIndex, uint32 vbOffset)
{
	if (!ValidateDrawcall())
	{
		return;
	}

#if DURANGO_ENABLE_ASYNC_DIPS
	if (CRenderer::CV_r_durango_async_dips)
	{
		if (s_CommandBuffer.m_current_offset * 4 >= SCommandBuffer::COMMAND_BUFFER_SIZE * 3)
		{
			gcpRendD3D.WaitForAsynchronousDevice();
			s_CommandBuffer.m_current_offset = 0;
		}

		char* start = &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset];
		CommitDeviceStatesAsync();

		SDrawIndexed* pJob = new(s_CommandBuffer.Allocate(sizeof(SDrawIndexed)))SDrawIndexed;
		pJob->NumIndices = numIndices;
		pJob->nIndexOffset = startIndex;
		pJob->nVertexOffset = vbOffset;

	#if CRY_PLATFORM_DURANGO
		SubmitAsyncDipJob(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
	#else
		TExecuteAsyncDIP dip;
		TExecuteAsyncDIPPacket packet(start, &s_CommandBuffer.m_buffer[s_CommandBuffer.m_current_offset], gcpRendD3D->GetAsynchronousDeviceState());
		s_dip_queue->AddPacket(packet, JobManager::eHighPriority, &dip, false);
	#endif // CRY_PLATFORM_DURANGO

		++m_nAsyncDipJobCounter;
		if (m_nAsyncDipJobCounter >= CRenderer::CV_r_durango_async_dips_sync)
		{
			gcpRendD3D->WaitForAsynchronousDevice();
			m_nAsyncDipJobCounter = 0;
		}

		return;
	}
#endif

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext_Unsynchronized().DrawIndexed(numIndices, startIndex, vbOffset);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::Dispatch(uint32 dX, uint32 dY, uint32 dZ)
{
	if (!ValidateDrawcall())
	{
		return;
	}

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext_Unsynchronized().Dispatch(dX, dY, dZ);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::DispatchIndirect(ID3D11Buffer* pBufferForArgs, uint32 AlignedOffsetForArgs)
{
	if (!ValidateDrawcall())
	{
		return;
	}

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext().DispatchIndirect(pBufferForArgs, AlignedOffsetForArgs);
	SELECT_ALL_GPU();
	SyncToGPU();
}

void CDeviceManager::DrawIndexedInstancedIndirect(ID3D11Buffer* pBufferForArgs, uint32 AlignedOffsetForArgs)
{
	if (!ValidateDrawcall())
	{
		return;
	}

	CommitDeviceStates();
	SELECT_GPU(gcpRendD3D->m_nGPU);
	gcpRendD3D->GetDeviceContext().DrawIndexedInstancedIndirect((ID3D11Buffer*)pBufferForArgs, AlignedOffsetForArgs);
	SELECT_ALL_GPU();
	SyncToGPU();
}

#define USE_BIT_TEST 1

#if !DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
void CDeviceManager::BindConstantBuffers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	#if USE_BIT_TEST
	using namespace DevManUtil;
	while (m_CB[type].dirty)
	{
		const unsigned lsb = m_CB[type].dirty & ~(m_CB[type].dirty - 1);
		const unsigned lbit = bsr(lsb);
		const unsigned hbit = bsf(~(m_CB[type].dirty) & ~(lsb - 1));
		const unsigned base = lbit;
		const unsigned count = hbit - lbit;
		switch (type)
		{
		case TYPE_VS:
			rDeviceContext.VSSetConstantBuffers(base, count, &m_CB[TYPE_VS].buffers[base]);
			break;
		case TYPE_PS:
			rDeviceContext.PSSetConstantBuffers(base, count, &m_CB[TYPE_PS].buffers[base]);
			break;
		case TYPE_GS:
			rDeviceContext.GSSetConstantBuffers(base, count, &m_CB[TYPE_GS].buffers[base]);
			break;
		case TYPE_DS:
			rDeviceContext.DSSetConstantBuffers(base, count, &m_CB[TYPE_DS].buffers[base]);
			break;
		case TYPE_HS:
			rDeviceContext.HSSetConstantBuffers(base, count, &m_CB[TYPE_HS].buffers[base]);
			break;
		case TYPE_CS:
			rDeviceContext.CSSetConstantBuffers(base, count, &m_CB[TYPE_CS].buffers[base]);
			break;
		}
		;
		const signed mask = iszero(32 - (signed)hbit) - 1;
		m_CB[type].dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
	}
	#else
	for (size_t i = 0; i < 32 && m_CB[type].dirty; ++i)
	{
		if (m_CB[type].dirty & (1 << i))
		{
			const unsigned base = i;
			const unsigned count = 1;
			switch (type)
			{
			case TYPE_VS:
				rDeviceContext.VSSetConstantBuffers(base, count, &m_CB[TYPE_VS].buffers[base]);
				break;
			case TYPE_PS:
				rDeviceContext.PSSetConstantBuffers(base, count, &m_CB[TYPE_PS].buffers[base]);
				break;
			case TYPE_GS:
				rDeviceContext.GSSetConstantBuffers(base, count, &m_CB[TYPE_GS].buffers[base]);
				break;
			case TYPE_DS:
				rDeviceContext.DSSetConstantBuffers(base, count, &m_CB[TYPE_DS].buffers[base]);
				break;
			case TYPE_HS:
				rDeviceContext.HSSetConstantBuffers(base, count, &m_CB[TYPE_HS].buffers[base]);
				break;
			case TYPE_CS:
				rDeviceContext.CSSetConstantBuffers(base, count, &m_CB[TYPE_CS].buffers[base]);
				break;
			}
			;
			m_CB[type].dirty &= ~(1 << i);
		}
	}
	#endif
}

void CDeviceManager::BindOffsetConstantBuffers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	#if defined(DEVICE_SUPPORTS_D3D11_1)
		#if USE_BIT_TEST
	using namespace DevManUtil;
	while (m_CB[type].dirty1)
	{
		const unsigned lsb = m_CB[type].dirty1 & ~(m_CB[type].dirty1 - 1);
		const unsigned lbit = bsr(lsb);
		const unsigned hbit = bsf(~(m_CB[type].dirty1) & ~(lsb - 1));
		const unsigned base = lbit;
		const unsigned count = hbit - lbit;
		switch (type)
		{
		case TYPE_VS:
			rDeviceContext.VSSetConstantBuffers1(base, count, &m_CB[TYPE_VS].buffers1[base], &m_CB[TYPE_VS].offsets[base], &m_CB[TYPE_VS].sizes[base]);
			break;
		case TYPE_PS:
			rDeviceContext.PSSetConstantBuffers1(base, count, &m_CB[TYPE_PS].buffers1[base], &m_CB[TYPE_PS].offsets[base], &m_CB[TYPE_PS].sizes[base]);
			break;
		case TYPE_GS:
			rDeviceContext.GSSetConstantBuffers1(base, count, &m_CB[TYPE_GS].buffers1[base], &m_CB[TYPE_GS].offsets[base], &m_CB[TYPE_GS].sizes[base]);
			break;
		case TYPE_DS:
			rDeviceContext.DSSetConstantBuffers1(base, count, &m_CB[TYPE_DS].buffers1[base], &m_CB[TYPE_DS].offsets[base], &m_CB[TYPE_DS].sizes[base]);
			break;
		case TYPE_HS:
			rDeviceContext.HSSetConstantBuffers1(base, count, &m_CB[TYPE_HS].buffers1[base], &m_CB[TYPE_HS].offsets[base], &m_CB[TYPE_HS].sizes[base]);
			break;
		case TYPE_CS:
			rDeviceContext.CSSetConstantBuffers1(base, count, &m_CB[TYPE_CS].buffers1[base], &m_CB[TYPE_CS].offsets[base], &m_CB[TYPE_CS].sizes[base]);
			break;
		}
		;
		const signed mask = iszero(32 - (signed)hbit) - 1;
		m_CB[type].dirty1 &= (~((1 << hbit) - (1 << lbit)) & mask);
	}
		#else
	for (size_t i = 0; i < 32 && m_CB[type].dirty1; ++i)
	{
		if (m_CB[type].dirty1 & (1 << i))
		{
			const unsigned base = i;
			const unsigned count = 1;
			switch (type)
			{
			case TYPE_VS:
				rDeviceContext.VSSetConstantBuffers1(base, count, &m_CB[TYPE_VS].buffers1[base], &m_CB[TYPE_VS].offsets[base], &m_CB[TYPE_VS].sizes[base]);
				break;
			case TYPE_PS:
				rDeviceContext.PSSetConstantBuffers1(base, count, &m_CB[TYPE_PS].buffers1[base], &m_CB[TYPE_PS].offsets[base], &m_CB[TYPE_PS].sizes[base]);
				break;
			case TYPE_GS:
				rDeviceContext.GSSetConstantBuffers1(base, count, &m_CB[TYPE_GS].buffers1[base], &m_CB[TYPE_GS].offsets[base], &m_CB[TYPE_GS].sizes[base]);
				break;
			case TYPE_DS:
				rDeviceContext.DSSetConstantBuffers1(base, count, &m_CB[TYPE_DS].buffers1[base], &m_CB[TYPE_DS].offsets[base], &m_CB[TYPE_DS].sizes[base]);
				break;
			case TYPE_HS:
				rDeviceContext.HSSetConstantBuffers1(base, count, &m_CB[TYPE_HS].buffers1[base], &m_CB[TYPE_HS].offsets[base], &m_CB[TYPE_HS].sizes[base]);
				break;
			case TYPE_CS:
				rDeviceContext.CSSetConstantBuffers1(base, count, &m_CB[TYPE_CS].buffers1[base], &m_CB[TYPE_CS].offsets[base], &m_CB[TYPE_CS].sizes[base]);
				break;
			}
			;
			m_CB[type].dirty1 &= ~(1 << i);
		}
	}
		#endif
	#endif // DEVICE_SUPPORTS_D3D11_1
}

void CDeviceManager::BindSamplers(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	#if USE_BIT_TEST
	while (m_Samplers[type].dirty)
	{
		using namespace DevManUtil;
		const unsigned lsb = m_Samplers[type].dirty & ~(m_Samplers[type].dirty - 1);
		const unsigned lbit = bsr(lsb);
		const unsigned hbit = bsf(~(m_Samplers[type].dirty) & ~(lsb - 1));
		const unsigned base = lbit;
		const unsigned count = hbit - lbit;
		switch (type)
		{
		case TYPE_VS:
			rDeviceContext.VSSetSamplers(base, count, &m_Samplers[TYPE_VS].samplers[base]);
			break;
		case TYPE_PS:
			rDeviceContext.PSSetSamplers(base, count, &m_Samplers[TYPE_PS].samplers[base]);
			break;
		case TYPE_GS:
			rDeviceContext.GSSetSamplers(base, count, &m_Samplers[TYPE_GS].samplers[base]);
			break;
		case TYPE_DS:
			rDeviceContext.DSSetSamplers(base, count, &m_Samplers[TYPE_DS].samplers[base]);
			break;
		case TYPE_HS:
			rDeviceContext.HSSetSamplers(base, count, &m_Samplers[TYPE_HS].samplers[base]);
			break;
		case TYPE_CS:
			rDeviceContext.CSSetSamplers(base, count, &m_Samplers[TYPE_CS].samplers[base]);
			break;
		}
		;
		const signed mask = iszero(32 - (signed)hbit) - 1;
		m_Samplers[type].dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
	}
	#else
	for (size_t i = 0; i < 32 && m_Samplers[type].dirty; ++i)
	{
		if (m_Samplers[type].dirty & (1 << i))
		{
			const unsigned base = i;
			const unsigned count = 1;
			switch (type)
			{
			case TYPE_VS:
				rDeviceContext.VSSetSamplers(base, count, &m_Samplers[TYPE_VS].samplers[base]);
				break;
			case TYPE_PS:
				rDeviceContext.PSSetSamplers(base, count, &m_Samplers[TYPE_PS].samplers[base]);
				break;
			case TYPE_GS:
				rDeviceContext.GSSetSamplers(base, count, &m_Samplers[TYPE_GS].samplers[base]);
				break;
			case TYPE_DS:
				rDeviceContext.DSSetSamplers(base, count, &m_Samplers[TYPE_DS].samplers[base]);
				break;
			case TYPE_HS:
				rDeviceContext.HSSetSamplers(base, count, &m_Samplers[TYPE_HS].samplers[base]);
				break;
			case TYPE_CS:
				rDeviceContext.CSSetSamplers(base, count, &m_Samplers[TYPE_CS].samplers[base]);
				break;
			}
			;
			m_Samplers[type].dirty &= ~(1 << i);
		}
	}
	#endif
}

void CDeviceManager::BindSRVs(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	for (size_t j = 0; j < MAX_SRV_DIRTY; ++j)
	{
	#if USE_BIT_TEST
		using namespace DevManUtil;
		while (m_SRV[type].dirty[j])
		{
			const unsigned lsb = m_SRV[type].dirty[j] & ~(m_SRV[type].dirty[j] - 1);
			const unsigned lbit = bsr(lsb);
			const unsigned hbit = bsf(~(m_SRV[type].dirty[j]) & ~(lsb - 1));
			const unsigned base = j * 32 + lbit;
			const unsigned count = hbit - lbit;
			switch (type)
			{
			case TYPE_VS:
				rDeviceContext.VSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			case TYPE_PS:
				rDeviceContext.PSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			case TYPE_GS:
				rDeviceContext.GSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			case TYPE_DS:
				rDeviceContext.DSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			case TYPE_HS:
				rDeviceContext.HSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			case TYPE_CS:
				rDeviceContext.CSSetShaderResources(base, count, &m_SRV[type].views[base]);
				break;
			}
			const signed mask = iszero(32 - (signed)hbit) - 1;
			m_SRV[type].dirty[j] &= (~((1 << hbit) - (1 << lbit)) & mask);
		}
	#else
		for (size_t i = 0; i < 32 && m_SRV[type].dirty[j]; ++i)
		{
			if (m_SRV[type].dirty[j] & (1 << i))
			{
				const unsigned base = j * 32 + i;
				const unsigned count = 1;
				switch (type)
				{
				case TYPE_VS:
					rDeviceContext.VSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				case TYPE_PS:
					rDeviceContext.PSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				case TYPE_GS:
					rDeviceContext.GSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				case TYPE_DS:
					rDeviceContext.DSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				case TYPE_HS:
					rDeviceContext.HSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				case TYPE_CS:
					rDeviceContext.CSSetShaderResources(base, count, &m_SRV[type].views[base]);
					break;
				}
				m_SRV[type].dirty[j] &= ~(1 << i);
			}
		}
	#endif
	}
}

void CDeviceManager::BindUAVs(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	for (size_t j = 0; j < MAX_UAV_DIRTY; ++j)
	{
	#if USE_BIT_TEST
		using namespace DevManUtil;
		while (m_UAV[type].dirty[j])
		{
			const unsigned lsb = m_UAV[type].dirty[j] & ~(m_UAV[type].dirty[j] - 1);
			const unsigned lbit = bsr(lsb);
			const unsigned hbit = bsf(~(m_UAV[type].dirty[j]) & ~(lsb - 1));
			const unsigned base = j * 32 + lbit;
			const unsigned count = hbit - lbit;
			switch (type)
			{
			case TYPE_VS:
				assert(0 && "NOT IMPLEMENTED ON D3D11.0");
				break;
			case TYPE_PS:
				rDeviceContext.OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
				break;
			case TYPE_GS:
				assert(0 && "NOT IMPLEMENTED ON D3D11.0");
				break;
			case TYPE_DS:
				assert(0 && "NOT IMPLEMENTED ON D3D11.0");
				break;
			case TYPE_HS:
				assert(0 && "NOT IMPLEMENTED ON D3D11.0");
				break;
			case TYPE_CS:
				rDeviceContext.CSSetUnorderedAccessViews(base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
				break;
			}
			const signed mask = iszero(32 - (signed)hbit) - 1;
			m_UAV[type].dirty[j] &= (~((1 << hbit) - (1 << lbit)) & mask);
		}
	#else
		for (size_t i = 0; i < 32 && m_UAV[type].dirty[j]; ++i)
		{
			if (m_UAV[type].dirty[j] & (1 << i))
			{
				const unsigned base = j * 32 + i;
				const unsigned count = 1;
				switch (type)
				{
				case TYPE_VS:
					assert(0 && "NOT IMPLEMENTED ON D3D11.0");
					break;
				case TYPE_PS:
					rDeviceContext.OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
					break;
				case TYPE_GS:
					assert(0 && "NOT IMPLEMENTED ON D3D11.0");
					break;
				case TYPE_DS:
					assert(0 && "NOT IMPLEMENTED ON D3D11.0");
					break;
				case TYPE_HS:
					assert(0 && "NOT IMPLEMENTED ON D3D11.0");
					break;
				case TYPE_CS:
					rDeviceContext.CSSetUnorderedAccessViews(base, count, &m_UAV[type].views[base], &m_UAV[type].counts[base]);
					break;
				}
				m_UAV[type].dirty[j] &= ~(1 << i);
			}
		}
	#endif
	}
}

void CDeviceManager::BindIA(CCryDeviceContextWrapper& rDeviceContext)
{
	#if USE_BIT_TEST
	using namespace DevManUtil;
	while (m_VBs.dirty)
	{
		const unsigned lsb = m_VBs.dirty & ~(m_VBs.dirty - 1);
		const unsigned lbit = bsr(lsb);
		const unsigned hbit = bsf(~(m_VBs.dirty) & ~(lsb - 1));
		const unsigned base = lbit;
		const unsigned count = hbit - lbit;
		const signed mask = iszero(32 - (signed)hbit) - 1;
		rDeviceContext.IASetVertexBuffers(base, count, &m_VBs.buffers[base], &m_VBs.strides[base], &m_VBs.offsets[base]);
		m_VBs.dirty &= (~((1 << hbit) - (1 << lbit)) & mask);
	}
	#else
	for (size_t i = 0; i < 32 && m_VBs.dirty; ++i)
	{
		if (m_VBs.dirty & (1 << i))
		{
			const unsigned base = i;
			const unsigned count = 1;
			rDeviceContext.IASetVertexBuffers(base, count, &m_VBs.buffers[base], &m_VBs.strides[base], &m_VBs.offsets[base]);
			m_VBs.dirty &= ~(1 << i);
		}
	}
	#endif

	if (m_IB.dirty)
	{
		rDeviceContext.IASetIndexBuffer(m_IB.buffer, (DXGI_FORMAT)m_IB.format, m_IB.offset);
		m_IB.dirty = 0;
	}
	if (m_VertexDecl.dirty)
	{
		rDeviceContext.IASetInputLayout(m_VertexDecl.decl);
		m_VertexDecl.dirty = false;
	}
	if (m_Topology.dirty)
	{
		rDeviceContext.IASetPrimitiveTopology(m_Topology.topology);
		m_Topology.dirty = false;
	}
}

void CDeviceManager::BindState(CCryDeviceContextWrapper& rDeviceContext)
{
	if (m_RasterState.dirty)
	{
		rDeviceContext.RSSetState(m_RasterState.pRasterizerState);
		m_RasterState.dirty = false;
	}
	if (m_BlendState.dirty)
	{
		rDeviceContext.OMSetBlendState(m_BlendState.pBlendState, m_BlendState.BlendFactor, m_BlendState.SampleMask);
		m_BlendState.dirty = false;
	}
	if (m_DepthStencilState.dirty)
	{
		rDeviceContext.OMSetDepthStencilState(m_DepthStencilState.dss, m_DepthStencilState.stencilref);
		m_DepthStencilState.dirty = false;
	}
}

void CDeviceManager::BindShader(SHADER_TYPE type, CCryDeviceContextWrapper& rDeviceContext)
{
	if (m_Shaders[type].dirty)
	{
		switch (type)
		{
		case TYPE_VS:
			rDeviceContext.VSSetShader((D3DVertexShader*)m_Shaders[type].shader, NULL, 0);
			break;
		case TYPE_PS:
			rDeviceContext.PSSetShader((D3DPixelShader*)m_Shaders[type].shader, NULL, 0);
			break;
		case TYPE_HS:
			rDeviceContext.HSSetShader((ID3D11HullShader*)m_Shaders[type].shader, NULL, 0);
			break;
		case TYPE_GS:
			rDeviceContext.GSSetShader((ID3D11GeometryShader*)m_Shaders[type].shader, NULL, 0);
			break;
		case TYPE_DS:
			rDeviceContext.DSSetShader((ID3D11DomainShader*)m_Shaders[type].shader, NULL, 0);
			break;
		case TYPE_CS:
			rDeviceContext.CSSetShader((ID3D11ComputeShader*)m_Shaders[type].shader, NULL, 0);
			break;
		}
		m_Shaders[type].dirty = false;
	}
}

void CDeviceManager::CommitDeviceStates()
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
	DETAILED_PROFILE_MARKER("CommitDeviceStates");

	#if DURANGO_ENABLE_ASYNC_DIPS
	gcpRendD3D->WaitForAsynchronousDevice();
	#endif
	{
		DETAILED_PROFILE_MARKER("InvalidateBuffers");
		const uint32 threadid = gRenDev->m_RP.m_nProcessThreadID;
		BufferInvalidationsT& invalidated = m_buffer_invalidations[threadid];

		for (size_t i = 0, end = invalidated.size(); i < end; ++i)
			CDeviceManager::InvalidateGpuCache(
			  invalidated[i].buffer,
			  invalidated[i].base_ptr,
			  invalidated[i].size,
			  invalidated[i].offset);

		invalidated.clear();
	}

	{
		DETAILED_PROFILE_MARKER("BindDeviceResources");
		CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
		BindIA(rDeviceContext);
		BindState(rDeviceContext);
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindShader(static_cast<SHADER_TYPE>(i), rDeviceContext);
		}
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindConstantBuffers(static_cast<SHADER_TYPE>(i), rDeviceContext);
		}
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindOffsetConstantBuffers(static_cast<SHADER_TYPE>(i), rDeviceContext);
		}
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindSRVs(static_cast<SHADER_TYPE>(i), rDeviceContext);
		}
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindUAVs((SHADER_TYPE)i, rDeviceContext);
		}
		for (signed i = 0; i < MAX_TYPE; ++i)
		{
			BindSamplers(static_cast<SHADER_TYPE>(i), rDeviceContext);
		}
	}
}
#endif

void CDeviceManager::SyncToGPU()
{
	if (CRenderer::CV_r_enable_full_gpu_sync)
	{
		if (m_fence_handle == DeviceFenceHandle() && FAILED(CreateFence(m_fence_handle)))
		{
			CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "could not create sync fence");
		}
		if (m_fence_handle)
		{
			IssueFence(m_fence_handle);
			SyncFence(m_fence_handle, true);
		}
	}
}

//=============================================================================

int CDeviceTexture::Cleanup()
{
	Unbind();

	int32 nRef = -1;
	if (m_pD3DTexture)
	{
#if !CRY_PLATFORM_ORBIS
		nRef = m_pD3DTexture->Release();
#else
		nRef = static_cast<D3DTexture*>(m_pD3DTexture)->Release();
#endif
		// NOTE: CDeviceTexture might be shared, take care the texture-pointer stays valid for the other aliases
		if (!nRef)
		{
			m_pD3DTexture = NULL;
		}
	}

	if (nRef != -1)
	{
#ifdef DEVMAN_USE_STAGING_POOL
		if (m_pStagingResource[0])
		{
			gcpRendD3D->m_DevMan.ReleaseStagingResource(m_pStagingResource[0]);
			m_pStagingResource[0] = NULL;
		}

		if (m_pStagingResource[1])
		{
			gcpRendD3D->m_DevMan.ReleaseStagingResource(m_pStagingResource[1]);
			m_pStagingResource[1] = NULL;
		}
#endif

#if CRY_PLATFORM_DURANGO
		if (nRef <= 0 && m_gpuHdl.IsValid())
		{
			gcpRendD3D->m_DevMan.m_texturePool.Free(m_gpuHdl);
			m_gpuHdl = SGPUMemHdl();
		}
#endif

#if defined(USE_NV_API)
		m_handleMGPU = NULL;
#endif
	}
	return nRef;
}

CDeviceTexture::~CDeviceTexture()
{
	Cleanup();
}

#ifndef CRY_PLATFORM_DURANGO
uint32 CDeviceTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF)
{
	return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF);
}
#endif

uint32 CDeviceTexture::TextureDataSize(D3DBaseView* pView)
{
	D3DResource* pResource = nullptr;
	pView->GetResource(&pResource);
	if (pResource)
	{
		uint32 nWidth = 0;
		uint32 nHeight = 0;
		uint32 nDepth = 0;
		uint32 nMips = 1;
		uint32 nSlices = 1;
		ETEX_Format eTF = eTF_Unknown;

		D3D11_RESOURCE_DIMENSION eResourceDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
		pResource->GetType(&eResourceDimension);
		pResource->Release();
		if (eResourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER)
		{
			D3D11_BUFFER_DESC sDesc;
			((ID3D11Buffer*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.ByteWidth;
			nHeight = 1;
			nDepth = 1;
			eTF = eTF_R8;
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
		{
			D3D11_TEXTURE1D_DESC sDesc;
			((ID3D11Texture1D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = 1;
			nDepth = 1;
			eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		{
			D3D11_TEXTURE2D_DESC sDesc;
			((ID3D11Texture2D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = sDesc.Height;
			nDepth = 1;
			eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
		{
			D3D11_TEXTURE3D_DESC sDesc;
			((ID3D11Texture3D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = sDesc.Height;
			nDepth = sDesc.Depth;
			eTF = CTexture::TexFormatFromDeviceFormat(sDesc.Format);
		}

		D3DUAV* pUAV = nullptr;
		D3DShaderResource* pSRV = nullptr;
		D3DDepthSurface* pDSV = nullptr;
		D3DSurface* pRSV = nullptr;
#if CRY_PLATFORM_ORBIS
		CCryDXOrbisResourceView* const pOrbisView = pView;
		switch (pOrbisView->Type())
		{
		case EDXORBIS_RT_UNORDEREDRESOURCEVIEW:
			pUAV = static_cast<CCryDXOrbisUnorderedAccessView*>(pView);
			pUAV->AddRef();
			break;
		case EDXORBIS_RT_SHADERRESOURCEVIEW:
			pSRV = static_cast<CCryDXOrbisShaderResourceView*>(pView);
			pSRV->AddRef();
			break;
		case EDXORBIS_RT_DEPTHSTENCILVIEW:
			pDSV = static_cast<CCryDXOrbisDepthStencilView*>(pView);
			pDSV->AddRef();
			break;
		case EDXORBIS_RT_RENDERTARGETVIEW:
			pRSV = static_cast<CCryDXOrbisRenderTargetView*>(pView);
			pRSV->AddRef();
			break;
		}
#elif !defined(OPENGL)
		pView->QueryInterface(__uuidof(ID3D11UnorderedAccessView), (void**)&pUAV);
		pView->QueryInterface(__uuidof(ID3D11ShaderResourceView), (void**)&pSRV);
		pView->QueryInterface(__uuidof(ID3D11DepthStencilView), (void**)&pDSV);
		pView->QueryInterface(__uuidof(ID3D11RenderTargetView), (void**)&pRSV);
#endif // !OPENGL

		if (pUAV)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC sUAVDesc;
			pUAV->GetDesc(&sUAVDesc);
			if (sUAVDesc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE1DARRAY)
			{
				nSlices = sUAVDesc.Texture1DArray.ArraySize;
			}
			else if (sUAVDesc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2DARRAY)
			{
				nSlices = sUAVDesc.Texture2DArray.ArraySize;
			}
			else if (sUAVDesc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE3D)
			{
				nSlices = sUAVDesc.Texture3D.WSize;
			}
		}

		// TODO: BUFFER doesn't have the sub-range calculated
		if (pSRV)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC sSRVDesc;
			pSRV->GetDesc(&sSRVDesc);
			if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1D)
			{
				nMips = sSRVDesc.Texture1D.MipLevels;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1DARRAY)
			{
				nMips = sSRVDesc.Texture1DArray.MipLevels;
				nSlices = sSRVDesc.Texture1DArray.ArraySize;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
			{
				nMips = sSRVDesc.Texture2D.MipLevels;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DARRAY)
			{
				nMips = sSRVDesc.Texture2DArray.MipLevels;
				nSlices = sSRVDesc.Texture2DArray.ArraySize;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY)
			{
				nSlices = sSRVDesc.Texture2DMSArray.ArraySize;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE)
			{
				nMips = sSRVDesc.TextureCube.MipLevels;
				nSlices = 6;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY)
			{
				nMips = sSRVDesc.TextureCubeArray.MipLevels;
				nSlices = sSRVDesc.TextureCubeArray.NumCubes * 6;
			}
			else if (sSRVDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE3D)
			{
				nMips = sSRVDesc.Texture3D.MipLevels;
			}
		}

		if (pDSV)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC sDSVDesc;
			pDSV->GetDesc(&sDSVDesc);
			if (sDSVDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE1DARRAY)
			{
				nSlices = sDSVDesc.Texture1DArray.ArraySize;
			}
			else if (sDSVDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY)
			{
				nSlices = sDSVDesc.Texture2DArray.ArraySize;
			}
			else if (sDSVDesc.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY)
			{
				nSlices = sDSVDesc.Texture2DMSArray.ArraySize;
			}
		}

		if (pRSV)
		{
			D3D11_RENDER_TARGET_VIEW_DESC sRSVDesc;
			pRSV->GetDesc(&sRSVDesc);
			if (sRSVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE1DARRAY)
			{
				nSlices = sRSVDesc.Texture1DArray.ArraySize;
			}
			else if (sRSVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY)
			{
				nSlices = sRSVDesc.Texture2DArray.ArraySize;
			}
			else if (sRSVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY)
			{
				nSlices = sRSVDesc.Texture2DMSArray.ArraySize;
			}
			else if (sRSVDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE3D)
			{
				nDepth = sRSVDesc.Texture3D.WSize;
			}
		}

		SAFE_RELEASE(pUAV);
		SAFE_RELEASE(pSRV);
		SAFE_RELEASE(pDSV);
		SAFE_RELEASE(pRSV);

		return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF);
	}

	return 0;
}

uint32 CDeviceTexture::TextureDataSize(D3DBaseView* pView, const uint numRects, const RECT* pRects)
{
	uint32 fullSize = TextureDataSize(pView);
	if (!numRects)
	{
		return fullSize;
	}

	D3DResource* pResource = nullptr;
	pView->GetResource(&pResource);
	if (pResource)
	{
		uint32 nWidth = 0;
		uint32 nHeight = 0;

		D3D11_RESOURCE_DIMENSION eResourceDimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
		pResource->GetType(&eResourceDimension);
		pResource->Release();
		if (eResourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER)
		{
			D3D11_BUFFER_DESC sDesc;
			((ID3D11Buffer*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.ByteWidth;
			nHeight = 1;
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
		{
			D3D11_TEXTURE1D_DESC sDesc;
			((ID3D11Texture1D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = 1;
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		{
			D3D11_TEXTURE2D_DESC sDesc;
			((ID3D11Texture2D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = sDesc.Height;
		}
		else if (eResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
		{
			D3D11_TEXTURE3D_DESC sDesc;
			((ID3D11Texture3D*)pResource)->GetDesc(&sDesc);
			nWidth = sDesc.Width;
			nHeight = sDesc.Height;
		}

		// If overlapping rectangles are cleared multiple times is ambiguous
		uint32 fullDim = nWidth * nHeight;
		uint32 rectSize = 0;

		for (uint i = 0; i < numRects; ++i)
		{
			uint32 nW = max(0, int32(min(uint32(pRects->right), nWidth)) - int32(max(0U, uint32(pRects->left))));
			uint32 nH = max(0, int32(min(uint32(pRects->bottom), nHeight)) - int32(max(0U, uint32(pRects->top))));

			uint32 rectDim = nW * nH;

			rectSize += (uint64(fullSize) * uint64(rectDim)) / fullDim;
		}

		return rectSize;
	}

	return 0;
}

//=============================================================================

alloc_info_struct* CRenderer::GetFreeChunk(int bytes_count, int nBufSize, PodArray<alloc_info_struct>& alloc_info, const char* szSource)
{
	int best_i = -1;
	int min_size = 10000000;

	// find best chunk
	for (int i = 0; i < alloc_info.Count(); i++)
	{
		if (!alloc_info[i].busy)
		{
			if (alloc_info[i].bytes_num >= bytes_count)
			{
				if (alloc_info[i].bytes_num < min_size)
				{
					best_i = i;
					min_size = alloc_info[i].bytes_num;
				}
			}
		}
	}

	if (best_i >= 0)
	{
		// use best free chunk
		alloc_info[best_i].busy = true;
		alloc_info[best_i].szSource = szSource;

		int bytes_free = alloc_info[best_i].bytes_num - bytes_count;
		if (bytes_free > 0)
		{
			// modify reused shunk
			alloc_info[best_i].bytes_num = bytes_count;

			// insert another free shunk
			alloc_info_struct new_chunk;
			new_chunk.bytes_num = bytes_free;
			new_chunk.ptr = alloc_info[best_i].ptr + alloc_info[best_i].bytes_num;
			new_chunk.busy = false;

			if (best_i < alloc_info.Count() - 1) // if not last
			{
				alloc_info.InsertBefore(new_chunk, best_i + 1);
			}
			else
			{
				alloc_info.Add(new_chunk);
			}
		}

		return &alloc_info[best_i];
	}

	int res_ptr = 0;

	int piplevel = alloc_info.Count() ? (alloc_info.Last().ptr - alloc_info[0].ptr) + alloc_info.Last().bytes_num : 0;
	if (piplevel + bytes_count >= nBufSize)
	{
		return NULL;
	}
	else
	{
		res_ptr = piplevel;
	}

	// register new chunk
	alloc_info_struct ai;
	ai.ptr = res_ptr;
	ai.szSource = szSource;
	ai.bytes_num = bytes_count;
	ai.busy = true;
	alloc_info.Add(ai);

	return &alloc_info[alloc_info.Count() - 1];
}

bool CRenderer::ReleaseChunk(int p, PodArray<alloc_info_struct>& alloc_info)
{
	for (int i = 0; i < alloc_info.Count(); i++)
	{
		if (alloc_info[i].ptr == p)
		{
			alloc_info[i].busy = false;

			// delete info about last unused chunks
			while (alloc_info.Count() && alloc_info.Last().busy == false)
			{
				alloc_info.Delete(alloc_info.Count() - 1);
			}

			// merge unused chunks
			for (int s = 0; s < alloc_info.Count() - 1; s++)
			{
				assert(alloc_info[s].ptr < alloc_info[s + 1].ptr);

				if (alloc_info[s].busy == false)
				{
					if (alloc_info[s + 1].busy == false)
					{
						alloc_info[s].bytes_num += alloc_info[s + 1].bytes_num;
						alloc_info.Delete(s + 1);
						s--;
					}
				}
			}

			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////
void SUsageTrackedItem::MarkUsed()
{
	m_lastUseFrame = gcpRendD3D->GetCurrentFrameCounter();
}

bool SUsageTrackedItem::IsInUse() const
{
	return m_lastUseFrame > gcpRendD3D->GetCompletedFrameCounter();
}

