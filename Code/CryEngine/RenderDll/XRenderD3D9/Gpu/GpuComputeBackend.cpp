// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuComputeBackend.h"
#include "DriverD3D.h"

namespace gpu
{

CounterReadbackUsed::CounterReadbackUsed()
{
	// count buffer
	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bufferDesc.MiscFlags = 0;
	bufferDesc.ByteWidth = 16;
	bufferDesc.StructureByteStride = sizeof(uint32);
#if CRY_PLATFORM_DURANGO && defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	HRESULT hr = D3DAllocateGraphicsMemory(4, 0, 0, D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT, &m_basePtr);
	gcpRendD3D.GetPerformanceDevice().CreatePlacementBuffer(&bufferDesc, m_basePtr, &buffer);
#else
	gcpRendD3D.GetDevice().CreateBuffer(&bufferDesc, nullptr, &buffer);
#endif
	m_countReadbackBuffer = buffer;
	m_readbackCalled = false;
}

int CounterReadbackUsed::Retrieve()
{
	if (!m_readbackCalled)
		return 0;

	int result = 0;
	CDeviceManager::DownloadContents<false>(m_countReadbackBuffer, 0, 0, sizeof(int), D3D11_MAP_READ, &result);

	m_readbackCalled = false;

	return result;
}

void CounterReadbackUsed::Readback(ID3D11UnorderedAccessView* uav)
{
	if (!uav)
		CryFatalError("UAV is 0");

	gcpRendD3D.GetDeviceContext().CopyStructureCount(m_countReadbackBuffer, 0, uav);
	m_readbackCalled = true;
}

DataReadbackUsed::DataReadbackUsed(int size, int stride)
	: m_readback(nullptr)
{
	// Create the Readback Buffer
	// This is used to read the results back to the CPU
	D3D11_BUFFER_DESC readback_buffer_desc;
	ZeroMemory(&readback_buffer_desc, sizeof(readback_buffer_desc));
	readback_buffer_desc.ByteWidth = size * stride;
	readback_buffer_desc.Usage = D3D11_USAGE_STAGING;
	readback_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readback_buffer_desc.StructureByteStride = stride;
#if CRY_PLATFORM_DURANGO && defined(DEVICE_SUPPORTS_PERFORMANCE_DEVICE)
	HRESULT hr = D3DAllocateGraphicsMemory(
	  size * stride, 0, 0, D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT, &m_basePtr);
	gcpRendD3D.GetPerformanceDevice().CreatePlacementBuffer(&readback_buffer_desc, m_basePtr, &m_readback);
#else
	gcpRendD3D.GetDevice().CreateBuffer(&readback_buffer_desc, nullptr, &m_readback);
#endif
	m_stride = stride;
	m_size = size;
	m_readbackCalled = false;
}

const void* DataReadbackUsed::Map(uint32 readLength)
{
	if (!m_readbackCalled)
		return nullptr;

	// Readback the data
	return CDeviceManager::Map(m_readback, 0, 0, readLength * m_stride, D3D11_MAP_READ);
}

void DataReadbackUsed::Unmap()
{
	CDeviceManager::Unmap(m_readback, 0, 0, 0, D3D11_MAP_READ);
	m_readbackCalled = false;
}

void DataReadbackUsed::Readback(ID3D11Buffer* buf, uint32 readLength)
{
	const D3D11_BOX readRange = { 0U, 0U, 0U, readLength * m_stride, 1U, 1U };
	gcpRendD3D.GetDeviceContext().CopySubresourceRegion(m_readback, 0, 0, 0, 0, buf, 0, &readRange);
	m_readbackCalled = true;
}
}
