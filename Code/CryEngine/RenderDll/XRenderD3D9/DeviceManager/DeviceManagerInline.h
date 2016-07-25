// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _DeviceManagerInline_H_
#define _DeviceManagerInline_H_

#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	#include "../DriverD3D.h"
#endif

#ifdef CRY_USE_DX12
#else
	#if CRY_PLATFORM_DURANGO
		#include "D3D11/DeviceManager_D3D11_Durango.h"
	#endif

	#if defined(USE_NV_API)
		#include "D3D11/DeviceManager_D3D11_NVAPI.h"
	#endif
#endif

//===============================================================================================================
//
//  Inline implementation for CDeviceManager
//
//===============================================================================================================

namespace DevManUtil
{
#if CRY_PLATFORM_ORBIS || CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE // BitScanReverse/BitScanForward are not avilable on orbis
static inline uint32 bsr(uint32 input)
{
	signed mask = iszero((signed)input) - 1;
	signed clz = __builtin_clz(input);
	signed index = 31 - clz;
	return (index & mask) | (32 & ~mask);
}
static inline uint32 bsf(uint32 input)
{
	signed mask = iszero((signed)input) - 1;
	signed ctz = __builtin_ctz(input);
	signed index = ctz;
	return (index & mask) | (32 & ~mask);
}
#else
static inline uint32 bsr(uint32 input)
{
	unsigned long index;
	signed result = BitScanReverse(&index, input);
	signed mask = iszero(result) - 1;
	return (index & mask) | (32 & ~mask);
}
static inline uint32 bsf(uint32 input)
{
	unsigned long index;
	signed result = BitScanForward(&index, input);
	signed mask = iszero(result) - 1;
	return (index & mask) | (32 & ~mask);
}
#endif
}

inline void CDeviceManager::BindConstantBuffer(SHADER_TYPE type, const CConstantBuffer* pBuffer, uint32 slot)
{
#if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS == 1
	BindConstantBuffer(type, pBuffer, slot, 0, 0);
	return;
#endif
	size_t _offset = 0, _size = 0;
	ID3D11Buffer* pBuf = pBuffer ? pBuffer->GetD3D(&_offset, &_size) : NULL;
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetConstantBuffers(slot, 1, &pBuf);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetConstantBuffers(slot, 1, &pBuf);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetConstantBuffers(slot, 1, &pBuf);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetConstantBuffers(slot, 1, &pBuf);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetConstantBuffers(slot, 1, &pBuf);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetConstantBuffers(slot, 1, &pBuf);
		break;
	}
#else
	m_CB[type].buffers[slot] = pBuf;
	m_CB[type].dirty |= 1 << slot;
#endif
}
inline void CDeviceManager::BindConstantBuffer(SHADER_TYPE type, const CConstantBuffer* pBuffer, uint32 slot, uint32 offset, uint32 size)
{
	size_t _offset = 0, _size = 0;
	ID3D11Buffer* pBuf = pBuffer ? pBuffer->GetD3D(&_offset, &_size) : NULL;

#if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS == 1
	_offset = (_offset + offset) >> 4;
	_size = size ? (size >> 4) : (_size >> 4);
	_size = max(_size, (size_t)16);
#else
	_offset = offset;
	_size = size;
#endif

#if !defined(DEVICE_SUPPORTS_D3D11_1)
	assert(_offset == 0);
	BindConstantBuffer(type, pBuffer, slot);
#elif DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	uint32 offsetArray[1] = { offset / sizeof(Vec4) };
	uint32 sizeArray[1] = { size / sizeof(Vec4) };
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetConstantBuffers1(slot, 1, &pBuf, offsetArray, sizeArray);
		break;
	}
#else
	m_CB[type].buffers1[slot] = pBuf;
	m_CB[type].offsets[slot] = _offset;
	m_CB[type].sizes[slot] = _size;
	m_CB[type].dirty1 |= 1 << slot;
#endif
}
inline void CDeviceManager::BindCBs(SHADER_TYPE type, const CConstantBuffer* const* Buffer, uint32 start_slot, uint32 count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		BindConstantBuffer(type, Buffer[i], start_slot + i);
	}
}
inline void CDeviceManager::BindConstantBuffer(SHADER_TYPE type, D3DBuffer* pBuffer, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetConstantBuffers(slot, 1, &pBuffer);
		break;
	}
#else
	m_CB[type].buffers[slot] = pBuffer;
	m_CB[type].dirty |= 1 << slot;
#endif
}
inline void CDeviceManager::BindConstantBuffer(SHADER_TYPE type, D3DBuffer* pBuffer, uint32 slot, uint32 offset, uint32 size)
{
	size_t _offset = 0, _size = 0;

#if CONSTANT_BUFFER_ENABLE_DIRECT_ACCESS == 1
	_offset = (_offset + offset) >> 4;
	_size = size ? (size >> 4) : (_size >> 4);
	_size = max(_size, (size_t)16);
#else
	_offset = offset;
	_size = size;
#endif

#if !defined(DEVICE_SUPPORTS_D3D11_1)
	assert(offset == 0);
	BindConstantBuffer(type, pBuffer, slot);
#elif DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	offset /= sizeof(Vec4);
	size /= sizeof(Vec4);
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetConstantBuffers1(slot, 1, &pBuffer, &offset, &size);
		break;
	}
#else
	m_CB[type].buffers1[slot] = pBuffer;
	m_CB[type].offsets[slot] = _offset;
	m_CB[type].sizes[slot] = _size;
	m_CB[type].dirty1 |= 1 << slot;
#endif
}
inline void CDeviceManager::BindCBs(SHADER_TYPE type, D3DBuffer* const* Buffer, uint32 start_slot, uint32 count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		BindConstantBuffer(type, Buffer[i], start_slot + i);
	}
}

inline void CDeviceManager::BindSRV(SHADER_TYPE type, D3DShaderResource* SRV, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetShaderResources(slot, 1, &SRV);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetShaderResources(slot, 1, &SRV);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetShaderResources(slot, 1, &SRV);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetShaderResources(slot, 1, &SRV);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetShaderResources(slot, 1, &SRV);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetShaderResources(slot, 1, &SRV);
		break;
	}
#else
	const unsigned dirty_base = slot >> SRV_DIRTY_SHIFT;
	const unsigned dirty_bit = slot & SRV_DIRTY_MASK;
	m_SRV[type].views[slot] = SRV;
	m_SRV[type].dirty[dirty_base] |= 1 << dirty_bit;
#endif
}
inline void CDeviceManager::BindSRV(SHADER_TYPE type, D3DShaderResource* const* SRV, uint32 start_slot, uint32 count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		BindSRV(type, SRV[i], i + start_slot);
	}
}
inline void CDeviceManager::BindUAV(SHADER_TYPE type, D3DUAV* UAV, uint32 count, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		assert(0 && "NOT IMPLEMENTED ON D3D11.0");
		break;
	case TYPE_PS:
		rDeviceContext.OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, NULL, NULL, slot, 1, &UAV, &count);
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
		rDeviceContext.CSSetUnorderedAccessViews(slot, 1, &UAV, &count);
		break;
	}
#else
	const unsigned dirty_base = slot >> UAV_DIRTY_SHIFT;
	const unsigned dirty_bit = slot & UAV_DIRTY_MASK;
	m_UAV[type].views[slot] = UAV;
	m_UAV[type].counts[slot] = count;
	m_UAV[type].dirty[dirty_base] |= 1 << dirty_bit;
#endif
}
inline void CDeviceManager::BindUAV(SHADER_TYPE type, D3DUAV* const* UAV, const uint32* counts, uint32 start_slot, uint32 count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		BindUAV(type, UAV[i], (counts ? counts[i] : -1), i + start_slot);
	}
}
inline void CDeviceManager::BindSampler(SHADER_TYPE type, D3DSamplerState* Sampler, uint32 slot)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetSamplers(slot, 1, &Sampler);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetSamplers(slot, 1, &Sampler);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetSamplers(slot, 1, &Sampler);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetSamplers(slot, 1, &Sampler);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetSamplers(slot, 1, &Sampler);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetSamplers(slot, 1, &Sampler);
		break;
	}
#else
	m_Samplers[type].samplers[slot] = Sampler;
	m_Samplers[type].dirty |= 1 << slot;
#endif
}
inline void CDeviceManager::BindSampler(SHADER_TYPE type, D3DSamplerState* const* Samplers, uint32 start_slot, uint32 count)
{
	for (unsigned i = 0; i < count; ++i)
	{
		BindSampler(type, Samplers[i], i + start_slot);
	}
}
inline void CDeviceManager::BindVB(D3DBuffer* Buffer, uint32 slot, uint32 offset, uint32 stride)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.IASetVertexBuffers(slot, 1, &Buffer, &stride, &offset);
#else
	m_VBs.buffers[slot] = Buffer;
	m_VBs.offsets[slot] = offset;
	m_VBs.strides[slot] = stride;
	m_VBs.dirty |= 1 << slot;
#endif
}
inline void CDeviceManager::BindVB(uint32 start, uint32 count, D3DBuffer* const* Buffers, const uint32* offset, const uint32* stride)
{
	for (size_t i = 0; i < count; ++i)
		BindVB(Buffers[i], start + i, offset[i], stride[i]);
}
inline void CDeviceManager::BindIB(D3DBuffer* Buffer, uint32 offset, DXGI_FORMAT format)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.IASetIndexBuffer(Buffer, format, offset);
#else
	m_IB.buffer = Buffer;
	m_IB.offset = offset;
	m_IB.format = format;
	m_IB.dirty = 1;
#endif
}
inline void CDeviceManager::BindVtxDecl(D3DVertexDeclaration* decl)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.IASetInputLayout(decl);
#else
	m_VertexDecl.decl = decl;
	m_VertexDecl.dirty = true;
#endif
}
inline void CDeviceManager::BindTopology(D3D11_PRIMITIVE_TOPOLOGY top)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.IASetPrimitiveTopology(top);
#else
	m_Topology.topology = top;
	m_Topology.dirty = true;
#endif
}
inline void CDeviceManager::BindShader(SHADER_TYPE type, ID3D11Resource* shader)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	switch (type)
	{
	case TYPE_VS:
		rDeviceContext.VSSetShader((D3DVertexShader*)shader, NULL, 0);
		break;
	case TYPE_PS:
		rDeviceContext.PSSetShader((D3DPixelShader*)shader, NULL, 0);
		break;
	case TYPE_HS:
		rDeviceContext.HSSetShader((ID3D11HullShader*)shader, NULL, 0);
		break;
	case TYPE_GS:
		rDeviceContext.GSSetShader((ID3D11GeometryShader*)shader, NULL, 0);
		break;
	case TYPE_DS:
		rDeviceContext.DSSetShader((ID3D11DomainShader*)shader, NULL, 0);
		break;
	case TYPE_CS:
		rDeviceContext.CSSetShader((ID3D11ComputeShader*)shader, NULL, 0);
		break;
	}
#else
	m_Shaders[type].shader = shader;
	m_Shaders[type].dirty = true;
#endif
}
inline void CDeviceManager::SetDepthStencilState(ID3D11DepthStencilState* dss, uint32 stencilref)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.OMSetDepthStencilState(dss, stencilref);
#else
	m_DepthStencilState.dss = dss;
	m_DepthStencilState.stencilref = stencilref;
	m_DepthStencilState.dirty = true;
#endif
}
inline void CDeviceManager::SetBlendState(ID3D11BlendState* pBlendState, float* BlendFactor, uint32 SampleMask)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.OMSetBlendState(pBlendState, BlendFactor, SampleMask);
#else
	m_BlendState.pBlendState = pBlendState;
	if (BlendFactor)
		for (size_t i = 0; i < 4; ++i)
			m_BlendState.BlendFactor[i] = BlendFactor[i];
	else
		for (size_t i = 0; i < 4; ++i)
			m_BlendState.BlendFactor[i] = 1.f;
	m_BlendState.SampleMask = SampleMask;
	m_BlendState.dirty = true;
#endif
}
inline void CDeviceManager::SetRasterState(ID3D11RasterizerState* pRasterizerState)
{
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
	CCryDeviceContextWrapper& rDeviceContext = gcpRendD3D->GetDeviceContext_Unsynchronized();
	rDeviceContext.RSSetState(pRasterizerState);
#else
	m_RasterState.pRasterizerState = pRasterizerState;
	m_RasterState.dirty = true;
#endif
}
#if DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
ILINE void CDeviceManager::CommitDeviceStates()
{
	// Nothing to do for DEVICE_MANAGER_IMMEDIATE_STATE_WRITE
}
#endif

//====================================================================================================

#endif  // _DeviceManager_H_
