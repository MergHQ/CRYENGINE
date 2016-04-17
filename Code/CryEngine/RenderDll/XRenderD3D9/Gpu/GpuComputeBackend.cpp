// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GpuComputeBackend.h"
#include "DriverD3D.h"

namespace gpu
{

CComputeBackend::CComputeBackend(const char* shaderName)
	: m_shaderName(shaderName)
	, m_pCurrentShader(nullptr)
{
}

CComputeBackend::~CComputeBackend()
{
}

bool CComputeBackend::RunTechnique(CCryNameTSCRC tech, UINT x, UINT y, UINT z, uint64 flags)
{
	SetFlags(flags);

	uint32 nPasses = 0;
	if (!m_pCurrentShader->FXSetTechnique(tech))
		return false;
	if (!m_pCurrentShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES))
		return false;
	if (!m_pCurrentShader->FXBeginPass(0))
		return false;

	gcpRendD3D.m_DevMan.Dispatch(x, y, z);

	if (!m_pCurrentShader->FXEndPass())
		return false;
	if (!m_pCurrentShader->FXEnd())
		return false;

	ID3D11UnorderedAccessView* pUAV0[8u] = { nullptr };
	SetUAVs(0u, 8u, pUAV0);
	ID3D11ShaderResourceView* pSRV0[8u] = { nullptr };
	SetSRVs(0u, 8u, pSRV0);

	// [PFX2_TODO_GPU]: Get rid of hardcoded bounds here
	ID3D11Buffer* bufs[16] = { nullptr };
	gcpRendD3D.m_DevMan.BindCBs(CDeviceManager::TYPE_CS, bufs, 4u, 9u);

	// to make sure everything is gone
	gcpRendD3D.m_DevMan.CommitDeviceStates();

	return true;
}

void CComputeBackend::SetUAVs(UINT offset, UINT numUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
	uint32 zeros[CDeviceManager::MAX_BOUND_UAVS] = { 0 };
	gcpRendD3D->m_DevMan.BindUAV(CDeviceManager::TYPE_CS, ppUnorderedAccessViews, zeros, offset, numUAVs);
}

void CComputeBackend::SetSRVs(UINT offset, UINT numSRVs, ID3D11ShaderResourceView** srv)
{
	gcpRendD3D.m_DevMan.BindSRV(CDeviceManager::TYPE_CS, srv, offset, numSRVs);
}

void CComputeBackend::SetFloatArray(CCryNameR name, Vec4* value, int number)
{
	m_pCurrentShader->FXSetCSFloat(name, value, number);
}

void CComputeBackend::SetFloat(CCryNameR name, Vec4 value)
{
	Vec4 f[] = { value };
	SetFloatArray(name, f, 1);
}

void CComputeBackend::SetFlags(uint64 flags)
{
	if (m_computeShaderMap.find(flags) == m_computeShaderMap.end())
	{
		m_computeShaderMap[flags] = gcpRendD3D.m_cEF.mfForName(m_shaderName.c_str(), EF_SYSTEM, 0, flags);
	}

	m_pCurrentShader = m_computeShaderMap[flags];
}

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
	bufferDesc.StructureByteStride = 4;
#ifdef DURANGO
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

	D3D11_MAPPED_SUBRESOURCE MappedResource = { 0 };

	int result = 0;
	{
		HRESULT res = gcpRendD3D.GetDeviceContext().Map(m_countReadbackBuffer, 0, D3D11_MAP_READ, 0L, &MappedResource);

		assert(MappedResource.pData);

		result = *(int*)MappedResource.pData;
		gcpRendD3D.GetDeviceContext().Unmap(m_countReadbackBuffer, 0);
	}

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
{
	// Create the Readback Buffer
	// This is used to read the results back to the CPU
	D3D11_BUFFER_DESC readback_buffer_desc;
	ZeroMemory(&readback_buffer_desc, sizeof(readback_buffer_desc));
	readback_buffer_desc.ByteWidth = size * stride;
	readback_buffer_desc.Usage = D3D11_USAGE_STAGING;
	readback_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	readback_buffer_desc.StructureByteStride = stride;
#ifdef DURANGO
	HRESULT hr = D3DAllocateGraphicsMemory(
	  size * stride, 0, 0, D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT, &m_basePtr);
	gcpRendD3D.GetPerformanceDevice().CreatePlacementBuffer(&readback_buffer_desc, m_basePtr, &m_readback);
#else
	gcpRendD3D.GetDevice().CreateBuffer(&readback_buffer_desc, nullptr, &m_readback);
#endif
	m_readbackCalled = false;
}

const void* DataReadbackUsed::Map()
{
	if (!m_readbackCalled)
		return nullptr;

	// Readback the data
	D3D11_MAPPED_SUBRESOURCE MappedResource = { 0 };
	HRESULT res = gcpRendD3D.GetDeviceContext().Map(m_readback, 0, D3D11_MAP_READ, 0L, &MappedResource);
	return MappedResource.pData;
}

void DataReadbackUsed::Unmap()
{
	gcpRendD3D.GetDeviceContext().Unmap(m_readback, 0);
	m_readbackCalled = false;
}

void DataReadbackUsed::Readback(ID3D11Buffer* buf)
{
	gcpRendD3D.GetDeviceContext().CopyResource(m_readback, buf);
	m_readbackCalled = true;
}
}
