// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:
//  Version:     v1.00
//  Created:     03/02/2015 by Jan Pinter
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef __CCRYDX12DEVICECONTEXT__
	#define __CCRYDX12DEVICECONTEXT__

	#include "DX12/CCryDX12Object.hpp"
	#include "DX12/Misc/SCryDX11PipelineState.hpp"

	#include "DX12/API/DX12Base.hpp"
	#include "DX12/API/DX12CommandList.hpp"

class CCryDX12Device;

class CCryDX12DeviceContext : public CCryDX12Object<ID3D11DeviceContext1>
{
	friend class CDeviceManager;
	friend class CDeviceObjectFactory;

public:
	DX12_OBJECT(CCryDX12DeviceContext, CCryDX12Object<ID3D11DeviceContext1> );

	static CCryDX12DeviceContext* Create(CCryDX12Device* pDevice, UINT nodeMask, bool isDeferred);

	CCryDX12DeviceContext(CCryDX12Device* pDevice, UINT nodeMask, bool isDeferred);

	virtual ~CCryDX12DeviceContext();

	ILINE bool IsDeferred() const
	{
		return m_bDeferred;
	}

	ILINE UINT GetNodeMask() const
	{
		return m_nodeMask;
	}

	bool RecreateCommandListPool(int nPoolId);

	ILINE void CeaseCoreCommandList(uint32 nPoolId)
	{
		CeaseCommandQueue(nPoolId, false);
	}

	ILINE void ResumeCoreCommandList(uint32 nPoolId)
	{
		ResumeCommandQueue(nPoolId);
	}

	ILINE NCryDX12::CCommandListPool& GetCoreCommandListPool(int nPoolId = CMDQUEUE_GRAPHICS)
	{
		return m_CmdListPools[nPoolId];
	}

	ILINE NCryDX12::CCommandList* GetCoreCommandList(int nPoolId = CMDQUEUE_GRAPHICS)
	{
		return m_pCmdLists[nPoolId];
	}

	ILINE NCryDX12::CCommandListPool& GetCoreGraphicsCommandListPool()
	{
		return m_CmdListPools[CMDQUEUE_GRAPHICS];
	}

	ILINE NCryDX12::CCommandList* GetCoreGraphicsCommandList() const
	{
		return m_pCmdLists[CMDQUEUE_GRAPHICS];
	}

	ILINE CCryDX12Device* GetDevice() const
	{
		return m_pDevice;
	}

	ILINE ID3D12Device* GetD3D12Device() const
	{
		return m_pDevice->GetD3D12Device();
	}

	ILINE CCryDX12DeviceContext* SetNode(UINT nodeMask) const
	{
		return GetDevice()->GetDeviceContext(nodeMask);
	}

	void Finish(NCryDX12::CSwapChain* pDX12SwapChain);
	void Finish();

	#pragma region /* ID3D11DeviceChild implementation */

	virtual void STDMETHODCALLTYPE GetDevice(
	  _Out_ ID3D11Device** ppDevice) final;

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
	  _In_ REFGUID guid,
	  _Inout_ UINT* pDataSize,
	  _Out_writes_bytes_opt_(*pDataSize)  void* pData) final;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
	  _In_ REFGUID guid,
	  _In_ UINT DataSize,
	  _In_reads_bytes_opt_(DataSize)  const void* pData) final;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
	  _In_ REFGUID guid,
	  _In_opt_ const IUnknown* pData) final;

	#pragma endregion

	#pragma region /* ID3D11DeviceContext implementation */

	virtual void STDMETHODCALLTYPE VSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE PSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE PSSetShader(
	  _In_opt_ ID3D11PixelShader * pPixelShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE PSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE VSSetShader(
	  _In_opt_ ID3D11VertexShader * pVertexShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE DrawIndexed(
	  _In_ UINT IndexCount,
	  _In_ UINT StartIndexLocation,
	  _In_ INT BaseVertexLocation) final;

	virtual void STDMETHODCALLTYPE Draw(
	  _In_ UINT VertexCount,
	  _In_ UINT StartVertexLocation) final;

	virtual HRESULT STDMETHODCALLTYPE Map(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource,
	  _In_ D3D11_MAP MapType,
	  _In_ UINT MapFlags,
	  _Out_ D3D11_MAPPED_SUBRESOURCE* pMappedResource) final;
	
	virtual HRESULT STDMETHODCALLTYPE Map(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource,
	  _In_ SIZE_T* BeginEnd,
	  _In_ D3D11_MAP MapType,
	  _In_ UINT MapFlags,
	  _Out_ D3D11_MAPPED_SUBRESOURCE* pMappedResource) final;

	virtual void STDMETHODCALLTYPE Unmap(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource) final;
	
	virtual void STDMETHODCALLTYPE Unmap(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource,
	  _In_ SIZE_T* BeginEnd) final;

	virtual void STDMETHODCALLTYPE PSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE IASetInputLayout(
	  _In_opt_ ID3D11InputLayout* pInputLayout) final;

	virtual void STDMETHODCALLTYPE IASetVertexBuffers(
	  _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppVertexBuffers,
	  _In_reads_opt_(NumBuffers)  const UINT * pStrides,
	  _In_reads_opt_(NumBuffers)  const UINT * pOffsets) final;

	virtual void STDMETHODCALLTYPE IASetIndexBuffer(
	  _In_opt_ ID3D11Buffer* pIndexBuffer,
	  _In_ DXGI_FORMAT Format,
	  _In_ UINT Offset) final;

	virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
	  _In_ UINT IndexCountPerInstance,
	  _In_ UINT InstanceCount,
	  _In_ UINT StartIndexLocation,
	  _In_ INT BaseVertexLocation,
	  _In_ UINT StartInstanceLocation) final;

	virtual void STDMETHODCALLTYPE DrawInstanced(
	  _In_ UINT VertexCountPerInstance,
	  _In_ UINT InstanceCount,
	  _In_ UINT StartVertexLocation,
	  _In_ UINT StartInstanceLocation) final;

	virtual void STDMETHODCALLTYPE GSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE GSSetShader(
	  _In_opt_ ID3D11GeometryShader * pShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
	  _In_ D3D11_PRIMITIVE_TOPOLOGY Topology) final;

	virtual void STDMETHODCALLTYPE VSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE VSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE Begin(
	  _In_ ID3D11Asynchronous* pAsync) final;

	virtual void STDMETHODCALLTYPE End(
	  _In_ ID3D11Asynchronous* pAsync) final;

	virtual HRESULT STDMETHODCALLTYPE GetData(
	  _In_ ID3D11Asynchronous * pAsync,
	  _Out_writes_bytes_opt_(DataSize)  void* pData,
	  _In_ UINT DataSize,
	  _In_ UINT GetDataFlags) final;

	virtual void STDMETHODCALLTYPE SetPredication(
	  _In_opt_ ID3D11Predicate* pPredicate,
	  _In_ BOOL PredicateValue) final;

	virtual void STDMETHODCALLTYPE GSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE GSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE OMSetRenderTargets(
	  _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11RenderTargetView * const* ppRenderTargetViews,
	  _In_opt_ ID3D11DepthStencilView * pDepthStencilView) final;

	virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(
	  _In_ UINT NumRTVs,
	  _In_reads_opt_(NumRTVs)  ID3D11RenderTargetView * const* ppRenderTargetViews,
	  _In_opt_ ID3D11DepthStencilView * pDepthStencilView,
	  _In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT UAVStartSlot,
	  _In_ UINT NumUAVs,
	  _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView * const* ppUnorderedAccessViews,
	  _In_reads_opt_(NumUAVs)  const UINT * pUAVInitialCounts) final;

	virtual void STDMETHODCALLTYPE OMSetBlendState(
	  _In_opt_ ID3D11BlendState* pBlendState,
	  _In_opt_ const FLOAT BlendFactor[4],
	  _In_ UINT SampleMask) final;

	virtual void STDMETHODCALLTYPE OMSetDepthStencilState(
	  _In_opt_ ID3D11DepthStencilState* pDepthStencilState,
	  _In_ UINT StencilRef) final;

	virtual void STDMETHODCALLTYPE SOSetTargets(
	  _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppSOTargets,
	  _In_reads_opt_(NumBuffers)  const UINT * pOffsets) final;

	virtual void STDMETHODCALLTYPE DrawAuto() final;

	virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(
	  _In_ ID3D11Buffer* pBufferForArgs,
	  _In_ UINT AlignedByteOffsetForArgs) final;

	virtual void STDMETHODCALLTYPE DrawInstancedIndirect(
	  _In_ ID3D11Buffer* pBufferForArgs,
	  _In_ UINT AlignedByteOffsetForArgs) final;

	virtual void STDMETHODCALLTYPE Dispatch(
	  _In_ UINT ThreadGroupCountX,
	  _In_ UINT ThreadGroupCountY,
	  _In_ UINT ThreadGroupCountZ) final;

	virtual void STDMETHODCALLTYPE DispatchIndirect(
	  _In_ ID3D11Buffer* pBufferForArgs,
	  _In_ UINT AlignedByteOffsetForArgs) final;

	virtual void STDMETHODCALLTYPE RSSetState(
	  _In_opt_ ID3D11RasterizerState* pRasterizerState) final;

	virtual void STDMETHODCALLTYPE RSSetViewports(
	  _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
	  _In_reads_opt_(NumViewports)  const D3D11_VIEWPORT * pViewports) final;

	virtual void STDMETHODCALLTYPE RSSetScissorRects(
	  _In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
	  _In_reads_opt_(NumRects)  const D3D11_RECT * pRects) final;

	virtual void STDMETHODCALLTYPE CopySubresourceRegion(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ UINT DstSubresource,
	  _In_ UINT DstX,
	  _In_ UINT DstY,
	  _In_ UINT DstZ,
	  _In_ ID3D11Resource* pSrcResource,
	  _In_ UINT SrcSubresource,
	  _In_opt_ const D3D11_BOX* pSrcBox) final;

	virtual void STDMETHODCALLTYPE CopySubresourcesRegion(
		_In_ ID3D11Resource* pDstResource,
		_In_ UINT DstSubresource,
		_In_ UINT DstX,
		_In_ UINT DstY,
		_In_ UINT DstZ,
		_In_ ID3D11Resource* pSrcResource,
		_In_ UINT SrcSubresource,
		_In_opt_ const D3D11_BOX* pSrcBox,
		_In_ UINT NumSubresources) final;

	virtual void STDMETHODCALLTYPE CopyResource(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ ID3D11Resource* pSrcResource) final;

	virtual void STDMETHODCALLTYPE UpdateSubresource(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ UINT DstSubresource,
	  _In_opt_ const D3D11_BOX* pDstBox,
	  _In_ const void* pSrcData,
	  _In_ UINT SrcRowPitch,
	  _In_ UINT SrcDepthPitch) final;

	virtual void STDMETHODCALLTYPE CopyStructureCount(
	  _In_ ID3D11Buffer* pDstBuffer,
	  _In_ UINT DstAlignedByteOffset,
	  _In_ ID3D11UnorderedAccessView* pSrcView) final;

	virtual void STDMETHODCALLTYPE ClearRenderTargetView(
	  _In_ ID3D11RenderTargetView* pRenderTargetView,
	  _In_ const FLOAT ColorRGBA[4]) final;

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(
	  _In_ ID3D11UnorderedAccessView* pUnorderedAccessView,
	  _In_ const UINT Values[4]) final;

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(
	  _In_ ID3D11UnorderedAccessView* pUnorderedAccessView,
	  _In_ const FLOAT Values[4]) final;

	virtual void STDMETHODCALLTYPE ClearDepthStencilView(
	  _In_ ID3D11DepthStencilView* pDepthStencilView,
	  _In_ UINT ClearFlags,
	  _In_ FLOAT Depth,
	  _In_ UINT8 Stencil) final;

	virtual void STDMETHODCALLTYPE GenerateMips(
	  _In_ ID3D11ShaderResourceView* pShaderResourceView) final;

	virtual void STDMETHODCALLTYPE SetResourceMinLOD(
	  _In_ ID3D11Resource* pResource,
	  FLOAT MinLOD) final;

	virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD(
	  _In_ ID3D11Resource* pResource) final;

	virtual void STDMETHODCALLTYPE ResolveSubresource(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ UINT DstSubresource,
	  _In_ ID3D11Resource* pSrcResource,
	  _In_ UINT SrcSubresource,
	  _In_ DXGI_FORMAT Format) final;

	virtual void STDMETHODCALLTYPE HSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE HSSetShader(
	  _In_opt_ ID3D11HullShader * pHullShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE HSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE HSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE DSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE DSSetShader(
	  _In_opt_ ID3D11DomainShader * pDomainShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE DSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE DSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE CSSetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _In_reads_opt_(NumViews)  ID3D11ShaderResourceView * const* ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(
	  _In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_1_UAV_SLOT_COUNT - StartSlot)  UINT NumUAVs,
	  _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView * const* ppUnorderedAccessViews,
	  _In_reads_opt_(NumUAVs)  const UINT * pUAVInitialCounts) final;

	virtual void STDMETHODCALLTYPE CSSetShader(
	  _In_opt_ ID3D11ComputeShader * pComputeShader,
	  _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance * const* ppClassInstances,
	  UINT NumClassInstances) final;

	virtual void STDMETHODCALLTYPE CSSetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _In_reads_opt_(NumSamplers)  ID3D11SamplerState * const* ppSamplers) final;

	virtual void STDMETHODCALLTYPE CSSetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _In_reads_opt_(NumBuffers)  ID3D11Buffer * const* ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE VSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE PSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE PSGetShader(
	  _Out_ ID3D11PixelShader** ppPixelShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE PSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE VSGetShader(
	  _Out_ ID3D11VertexShader** ppVertexShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE PSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE IAGetInputLayout(
	  _Out_ ID3D11InputLayout** ppInputLayout) final;

	virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
	  _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppVertexBuffers,
	  _Out_writes_opt_(NumBuffers)  UINT * pStrides,
	  _Out_writes_opt_(NumBuffers)  UINT * pOffsets) final;

	virtual void STDMETHODCALLTYPE IAGetIndexBuffer(
	  _Out_opt_ ID3D11Buffer** pIndexBuffer,
	  _Out_opt_ DXGI_FORMAT* Format,
	  _Out_opt_ UINT* Offset) final;

	virtual void STDMETHODCALLTYPE GSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE GSGetShader(
	  _Out_ ID3D11GeometryShader** ppGeometryShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(
	  _Out_ D3D11_PRIMITIVE_TOPOLOGY* pTopology) final;

	virtual void STDMETHODCALLTYPE VSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE VSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE GetPredication(
	  _Out_opt_ ID3D11Predicate** ppPredicate,
	  _Out_opt_ BOOL* pPredicateValue) final;

	virtual void STDMETHODCALLTYPE GSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE GSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE OMGetRenderTargets(
	  _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11RenderTargetView * *ppRenderTargetViews,
	  _Out_opt_ ID3D11DepthStencilView * *ppDepthStencilView) final;

	virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(
	  _In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumRTVs,
	  _Out_writes_opt_(NumRTVs)  ID3D11RenderTargetView * *ppRenderTargetViews,
	  _Out_opt_ ID3D11DepthStencilView * *ppDepthStencilView,
	  _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT UAVStartSlot,
	  _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  UINT NumUAVs,
	  _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView * *ppUnorderedAccessViews) final;

	virtual void STDMETHODCALLTYPE OMGetBlendState(
	  _Out_opt_ ID3D11BlendState * *ppBlendState,
	  _Out_opt_ FLOAT BlendFactor[4],
	  _Out_opt_ UINT * pSampleMask) final;

	virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
	  _Out_opt_ ID3D11DepthStencilState** ppDepthStencilState,
	  _Out_opt_ UINT* pStencilRef) final;

	virtual void STDMETHODCALLTYPE SOGetTargets(
	  _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppSOTargets) final;

	virtual void STDMETHODCALLTYPE RSGetState(
	  _Out_ ID3D11RasterizerState** ppRasterizerState) final;

	virtual void STDMETHODCALLTYPE RSGetViewports(
	  _Inout_ UINT* pNumViewports,
	  _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT* pViewports) final;

	virtual void STDMETHODCALLTYPE RSGetScissorRects(
	  _Inout_ UINT* pNumRects,
	  _Out_writes_opt_(*pNumRects)  D3D11_RECT* pRects) final;

	virtual void STDMETHODCALLTYPE HSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE HSGetShader(
	  _Out_ ID3D11HullShader** ppHullShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE HSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE HSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE DSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE DSGetShader(
	  _Out_ ID3D11DomainShader** ppDomainShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE DSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE DSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE CSGetShaderResources(
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
	  _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView * *ppShaderResourceViews) final;

	virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(
	  _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  UINT NumUAVs,
	  _Out_writes_opt_(NumUAVs)  ID3D11UnorderedAccessView * *ppUnorderedAccessViews) final;

	virtual void STDMETHODCALLTYPE CSGetShader(
	  _Out_ ID3D11ComputeShader** ppComputeShader,
	  _Out_writes_opt_(*pNumClassInstances)  ID3D11ClassInstance** ppClassInstances,
	  _Inout_opt_ UINT* pNumClassInstances) final;

	virtual void STDMETHODCALLTYPE CSGetSamplers(
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
	  _Out_writes_opt_(NumSamplers)  ID3D11SamplerState * *ppSamplers) final;

	virtual void STDMETHODCALLTYPE CSGetConstantBuffers(
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
	  _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
	  _Out_writes_opt_(NumBuffers)  ID3D11Buffer * *ppConstantBuffers) final;

	virtual void STDMETHODCALLTYPE                      ClearState() final;

	virtual void STDMETHODCALLTYPE                      Flush() final;

	virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType() final;

	virtual UINT STDMETHODCALLTYPE                      GetContextFlags() final;

	virtual void STDMETHODCALLTYPE                      ExecuteCommandList(
	  _In_ ID3D11CommandList* pCommandList,
	  BOOL RestoreContextState) final;

	virtual HRESULT STDMETHODCALLTYPE FinishCommandList(
	  _In_ BOOL RestoreDeferredContextState,
	  _Out_opt_ ID3D11CommandList** ppCommandList) final;

	#pragma endregion

	#pragma region /* D3D 11.1 specific functions */

	virtual void STDMETHODCALLTYPE CopySubresourceRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags) final;
	virtual void STDMETHODCALLTYPE CopySubresourcesRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags, UINT NumSubresource) final;
	virtual void STDMETHODCALLTYPE CopyResource1(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource, UINT CopyFlags) final;
	virtual void STDMETHODCALLTYPE UpdateSubresource1(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags) final;
	virtual void STDMETHODCALLTYPE DiscardResource(ID3D11Resource* pResource) final;
	virtual void STDMETHODCALLTYPE DiscardView(ID3D11View* pResourceView) final;

	virtual void STDMETHODCALLTYPE VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants) final;

	virtual void STDMETHODCALLTYPE VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;
	virtual void STDMETHODCALLTYPE CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants) final;

	virtual void STDMETHODCALLTYPE SwapDeviceContextState(ID3DDeviceContextState* pState, ID3DDeviceContextState** ppPreviousState) final;
	virtual void STDMETHODCALLTYPE ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects) final;
	virtual void STDMETHODCALLTYPE DiscardView1(ID3D11View* pResourceView, const D3D11_RECT* pRects, UINT NumRects) final;

	#pragma endregion

	// Clear rectangle API
	void STDMETHODCALLTYPE ClearRectsRenderTargetView(
	  _In_ ID3D11RenderTargetView * pRenderTargetView,
	  _In_  const FLOAT ColorRGBA[4],
	  _In_ UINT NumRects,
	  _In_reads_opt_(NumRects)  const D3D11_RECT * pRects);

	void STDMETHODCALLTYPE ClearRectsUnorderedAccessViewUint(
	  _In_ ID3D11UnorderedAccessView * pUnorderedAccessView,
	  _In_  const UINT Values[4],
	  _In_ UINT NumRects,
	  _In_reads_opt_(NumRects)  const D3D11_RECT * pRects);

	void STDMETHODCALLTYPE ClearRectsUnorderedAccessViewFloat(
	  _In_ ID3D11UnorderedAccessView * pUnorderedAccessView,
	  _In_  const FLOAT Values[4],
	  _In_ UINT NumRects,
	  _In_reads_opt_(NumRects)  const D3D11_RECT * pRects);

	void STDMETHODCALLTYPE ClearRectsDepthStencilView(
	  _In_ ID3D11DepthStencilView * pDepthStencilView,
	  _In_ UINT ClearFlags,
	  _In_ FLOAT Depth,
	  _In_ UINT8 Stencil,
	  _In_ UINT NumRects,
	  _In_reads_opt_(NumRects)  const D3D11_RECT * pRects);

	// Cross-node multi-adapter API
	void STDMETHODCALLTYPE CopyResourceOvercross(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ ID3D11Resource* pSrcResource);

	void STDMETHODCALLTYPE CopyResourceOvercross1(
		_In_ ID3D11Resource* pDstResource,
		_In_ ID3D11Resource* pSrcResource,
		_In_ UINT CopyFlags);

	void STDMETHODCALLTYPE JoinSubresourceRegion(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ UINT DstSubresource,
	  _In_ UINT DstX,
	  _In_ UINT DstY,
	  _In_ UINT DstZ,
	  _In_ ID3D11Resource* pSrcResourceL,
	  _In_ ID3D11Resource* pSrcResourceR,
	  _In_ UINT SrcSubresource,
	  _In_opt_ const D3D11_BOX* pSrcBox);

	void STDMETHODCALLTYPE JoinSubresourceRegion1(
	  _In_ ID3D11Resource* pDstResource,
	  _In_ UINT DstSubresource,
	  _In_ UINT DstX,
	  _In_ UINT DstY,
	  _In_ UINT DstZ,
	  _In_ ID3D11Resource* pSrcResourceL,
	  _In_ ID3D11Resource* pSrcResourceR,
	  _In_ UINT SrcSubresource,
	  _In_opt_ const D3D11_BOX* pSrcBox,
	  _In_ UINT CopyFlags);

	// Staging resource API
	HRESULT STDMETHODCALLTYPE CopyStagingResource(
	  _In_ ID3D11Resource* pStagingResource,
	  _In_ ID3D11Resource* pSourceResource,
	  _In_ UINT SubResource,
	  _In_ BOOL Upload);

	HRESULT STDMETHODCALLTYPE TestStagingResource(
	  _In_ ID3D11Resource* pStagingResource);

	HRESULT STDMETHODCALLTYPE WaitStagingResource(
	  _In_ ID3D11Resource* pStagingResource);

	HRESULT STDMETHODCALLTYPE MapStagingResource(
	  _In_ ID3D11Resource* pStagingResource,
	  _In_ BOOL Upload,
	  _Out_ void** ppStagingMemory);

	void STDMETHODCALLTYPE UnmapStagingResource(
	  _In_ ID3D11Resource* pStagingResource,
	  _In_ BOOL Upload);

	HRESULT STDMETHODCALLTYPE MappedWriteToSubresource(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource,
	  _In_ SIZE_T Offset,
	  _In_ SIZE_T Size,
	  _In_ D3D11_MAP MapType,
	  _In_reads_bytes_(Size) const void* pData,
	  _In_ const UINT numDataBlocks);

	HRESULT STDMETHODCALLTYPE MappedReadFromSubresource(
	  _In_ ID3D11Resource* pResource,
	  _In_ UINT Subresource,
	  _In_ SIZE_T Offset,
	  _In_ SIZE_T Size,
	  _In_ D3D11_MAP MapType,
	  _Out_writes_bytes_(Size) void* pData,
	  _In_ const UINT numDataBlocks);

	// Upload resource API
	void STDMETHODCALLTYPE UploadResource(
	  _In_ ICryDX12Resource * pDstResource,
	  _In_ size_t NumInitialData,
	  _In_reads_(NumInitialData) const D3D11_SUBRESOURCE_DATA * pInitialData);

	void STDMETHODCALLTYPE UploadResource(
	  _In_ NCryDX12::CResource* pDstResource,
	  _In_ const NCryDX12::CResource::SInitialData* pSrcData);

	void STDMETHODCALLTYPE UpdateSubresources(
	  _In_ NCryDX12::CResource * pDstResource,
	  _In_ UINT64 UploadBufferSize,
	  _In_ UINT NumInitialData,
	  _In_reads_(NumInitialData) D3D12_SUBRESOURCE_DATA * pSrcData);

	// Timestamp management API
	ILINE void ResetTimestamps()
	{
		m_TimestampIndex = 0;
	}

	ILINE UINT64 GetTimestampFrequency()
	{
		// D3D12 WARNING: ID3D12CommandQueue::ID3D12CommandQueue::GetTimestampFrequency: Use
		// ID3D12Device::SetStablePowerstate for reliable timestamp queries [ EXECUTION WARNING #736: UNSTABLE_POWER_STATE]
		UINT64 Frequency = 0;
		m_pCmdLists[CMDQUEUE_GRAPHICS]->GetD3D12CommandQueue()->GetTimestampFrequency(&Frequency);
		return Frequency;
	}

	ILINE INT ReserveTimestamp()
	{
		return TimestampIndex(m_pCmdLists[CMDQUEUE_GRAPHICS]);
	}

	ILINE void InsertTimestamp(INT index, INT commandQueue)
	{
		assert(commandQueue >= CMDQUEUE_GRAPHICS && commandQueue <= CMDQUEUE_COPY);
		InsertTimestamp(m_pCmdLists[commandQueue], index);
	}

	ILINE void ResolveTimestamps()
	{
		ResolveTimestamps(m_pCmdLists[CMDQUEUE_GRAPHICS]);
	}

	ILINE void InsertTimestamp(INT index, void* mem)
	{
		QueryTimestamp(m_pCmdLists[CMDQUEUE_GRAPHICS], index, mem);
	}

	ILINE void QueryTimestamps(INT firstIndex, INT numIndices, void* mem)
	{
		uint32 outIndex = 0;
		for (INT index = firstIndex; index < firstIndex + numIndices; ++index)
		{
			QueryTimestamp(m_pCmdLists[CMDQUEUE_GRAPHICS], index, (char*)mem + outIndex * sizeof(UINT64));
			++outIndex;
		}
	}

	ILINE void QueryTimestampBuffer(ID3D11Buffer** ppTimestampBuffer)
	{
		if (ppTimestampBuffer)
		{
			*ppTimestampBuffer = CCryDX12Buffer::Create(m_pDevice, m_TimestampDownloadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		}
	}

	// Fence management API
	ILINE UINT64 InsertFence()
	{
		return m_CmdFenceSet.GetCurrentValue(CMDQUEUE_GRAPHICS) - !m_pCmdLists[CMDQUEUE_GRAPHICS]->IsUtilized();
	}

	ILINE HRESULT FlushToFence(UINT64 fenceValue)
	{
		SubmitCommands(CMDQUEUE_GRAPHICS, false, fenceValue);
		m_CmdListPools[CMDQUEUE_GRAPHICS].GetAsyncCommandQueue().Flush(fenceValue);
		return S_OK;
	}

	ILINE HRESULT TestForFence(UINT64 fenceValue)
	{
		return m_CmdFenceSet.IsCompleted(fenceValue, CMDQUEUE_GRAPHICS) ? S_OK : S_FALSE;
	}

	ILINE HRESULT WaitForFence(UINT64 fenceValue)
	{
		m_CmdFenceSet.WaitForFence(fenceValue, CMDQUEUE_GRAPHICS);
		return S_OK;
	}

	// Misc
	void ResetCachedState(bool bGraphics = true, bool bCompute = false);

private:
	bool PrepareComputePSO();
	bool PrepareGraphicsPSO();
	void PrepareGraphicsFF();
	void PrepareIO(bool bGfx);

	bool PrepareGraphicsState();
	bool PrepareComputeState();

	void CeaseCommandQueue(int nPoolId, bool wait);
	void ResumeCommandQueue(int nPoolId);
	void CeaseAllCommandQueues(bool wait);
	void ResumeAllCommandQueues();

	void SubmitCommands(int nPoolId, bool wait);
	void SubmitCommands(int nPoolId, bool wait, const UINT64 fenceValue);
	void SubmitAllCommands(bool wait);
	void SubmitAllCommands(bool wait, const UINT64 (&fenceValues)[CMDQUEUE_NUM]);
	void SubmitAllCommands(bool wait, const NCryDX12::FVAL64 (&fenceValues)[CMDQUEUE_NUM]);

	void BindResources(bool bGfx);
	void BindOutputViews();

	void DebugPrintResources(bool bGfx);

	void SetConstantBuffers1(NCryDX12::EShaderStage shaderStage, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants, bool bGfx);

	bool               m_bDeferred;
	UINT               m_nodeMask;

	CCryDX12Device*    m_pDevice;
	NCryDX12::CDevice* m_pDX12Device;

	#define FRAME_FENCES         32
	#define FRAME_FENCE_LATENCY  32
	#define FRAME_FENCE_INFLIGHT MAX_FRAMES_IN_FLIGHT
	UINT64                                  m_FrameFenceValuesSubmitted[FRAME_FENCES][CMDQUEUE_NUM];
	UINT64                                  m_FrameFenceValuesCompleted[FRAME_FENCES][CMDQUEUE_NUM];
	ULONG                                   m_FrameFenceCursor;

	NCryDX12::CCommandListFenceSet          m_CmdFenceSet;
	#if defined(_ALLOW_INITIALIZER_LISTS)
	NCryDX12::CCommandListPool              m_CmdListPools[CMDQUEUE_NUM];
	#else
	std::vector<NCryDX12::CCommandListPool> m_CmdListPools;
	#endif
	DX12_PTR(NCryDX12::CCommandList) m_pCmdLists[CMDQUEUE_NUM];
	bool m_bCmdListBegins[CMDQUEUE_NUM];

	DX12_PTR(NCryDX12::CPSO) m_pCurrentPSO;
	DX12_PTR(NCryDX12::CRootSignature) m_pCurrentRootSignature;

	SCryDX11PipelineState m_GraphicsState;
	SCryDX11PipelineState m_ComputeState;

public:
	UINT TimestampIndex(NCryDX12::CCommandList* pCmdList);
	void InsertTimestamp(NCryDX12::CCommandList* pCmdList, UINT index);
	void ResolveTimestamps(NCryDX12::CCommandList* pCmdList);
	void QueryTimestamp(NCryDX12::CCommandList* pCmdList, UINT index, void* mem);

	UINT OcclusionIndex(NCryDX12::CCommandList* pCmdList, bool counter);
	void InsertOcclusionStart(NCryDX12::CCommandList* pCmdList, UINT index, bool counter);
	void InsertOcclusionStop(NCryDX12::CCommandList* pCmdList, UINT index, bool counter);
	void ResolveOcclusion(NCryDX12::CCommandList* pCmdList, UINT index, void* mem);

	void PushProfilerEvent(const char* label);
	void PopProfilerEvent();

private:
	ID3D12Resource*            m_TimestampDownloadBuffer;
	ID3D12Resource*            m_OcclusionDownloadBuffer;
	UINT                       m_TimestampIndex;
	UINT                       m_OcclusionIndex;
	void*                      m_TimestampMemory;
	void*                      m_OcclusionMemory;
	bool                       m_TimestampMapValid;
	bool                       m_OcclusionMapValid;
	NCryDX12::CDescriptorHeap* m_pResourceHeap;
	NCryDX12::CDescriptorHeap* m_pSamplerHeap;
	DynArray<SSamplerGroup>    m_SamplerGroups;
	int                        m_nResDynOffset;
	int                        m_nFrame;
	uint32                     m_nStencilRef;

	bool                       m_bBatchComputeTasks;

	NCryDX12::CQueryHeap       m_TimestampHeap;
	NCryDX12::CQueryHeap       m_OcclusionHeap;

	DynArray<const char*>      m_profilerEventStack;

#ifdef DX12_STATS
	size_t m_NumMapDiscardSkips;
	size_t m_NumMapDiscards;
	size_t m_NumCopyDiscardSkips;
	size_t m_NumCopyDiscards;

	size_t m_NumCommandListOverflows;
	size_t m_NumCommandListSplits;
#endif // DX12_STATS
};

#endif // __CCRYDX12DEVICECONTEXT__
