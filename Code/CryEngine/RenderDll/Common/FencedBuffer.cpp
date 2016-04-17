// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FencedBuffer.h"
#include "../XRenderD3D9/DriverD3D.h"

FencedBuffer::FencedBuffer(uint elemsCount, uint stride, DXGI_FORMAT format, uint32 flags)
	: m_pLockedData(nullptr)
	, m_elemCount(elemsCount)
	, m_stride(stride)
	, m_flags(flags)
	, m_buffer(1)
{
	m_buffer.Create(elemsCount, stride, format, flags, nullptr);
	gRenDev->m_DevMan.CreateFence(m_fence);
}

FencedBuffer::~FencedBuffer()
{
	Unlock();
	m_buffer.Release();
	gRenDev->m_DevMan.ReleaseFence(m_fence);
}

byte* FencedBuffer::Lock()
{
	if (m_pLockedData)
		return m_pLockedData;

	if (m_buffer.GetBuffer())
		m_pLockedData = reinterpret_cast<byte*>(m_buffer.Lock());

	return m_pLockedData;
}

void FencedBuffer::Unlock()
{
	if (m_pLockedData && m_buffer.GetBuffer())
	{
		m_buffer.Unlock();
		// (1074511) !RB durango: removed usage of D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_NONCOHERENT from all created buffers as advised by microsoft. All dynamic buffers now use onion (D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT) and all static buffers use garlic now (D3D11_GRAPHICS_MEMORY_ACCESS_CPU_WRITECOMBINE_NONCOHERENT)
#if !CRY_PLATFORM_DURANGO
		CDeviceManager::InvalidateCpuCache(m_pLockedData, 0, m_elemCount * m_stride);
		CDeviceManager::InvalidateGpuCache(m_buffer.GetBuffer(), m_pLockedData, 0, m_elemCount * m_stride);
#endif
		m_pLockedData = nullptr;
	}
}

HRESULT FencedBuffer::BindVB(uint streamNumber, int bytesOffset, int stride)
{
	assert((m_flags & DX11BUF_BIND_VERTEX_BUFFER) != 0);
	HRESULT h = gcpRendD3D->FX_SetVStream(
	  streamNumber, m_buffer.GetBuffer(), bytesOffset,
	  (stride == 0 ? m_stride : stride));
	CHECK_HRESULT(h);
	return h;
}

HRESULT FencedBuffer::BindIB(uint offset)
{
	assert((m_flags & DX11BUF_BIND_INDEX_BUFFER) != 0);
	return gcpRendD3D->FX_SetIStream(
	  m_buffer.GetBuffer(), offset, (m_stride == 2 ? Index16 : Index32));
}

void FencedBuffer::BindSRV(CDeviceManager::SHADER_TYPE shaderType, uint slot)
{
	assert(m_buffer.GetSRV() != 0);
	auto pSRV = m_buffer.GetSRV();
	gcpRendD3D->m_DevMan.BindSRV(shaderType, &pSRV, slot, 1);
}

void FencedBuffer::SetFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
	gRenDev->m_DevMan.IssueFence(m_fence);
#endif
}

void FencedBuffer::WaitForFence()
{
#if BUFFER_ENABLE_DIRECT_ACCESS == 1
	gRenDev->m_DevMan.SyncFence(m_fence, true, false);
#endif
}
