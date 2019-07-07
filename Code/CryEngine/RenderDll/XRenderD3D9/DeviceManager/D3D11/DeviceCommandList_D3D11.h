// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CDeviceGraphicsCommandInterfaceImpl;
class CDeviceComputeCommandInterfaceImpl;
class CDeviceNvidiaCommandInterfaceImpl;

class CDeviceCopyCommandInterfaceImpl;
class CDeviceRenderPass;

namespace NCryDX11
{
	class CCommandList;
}

struct SSharedState
{
	DX11_PTR(NCryDX11::CCommandList) pCommandList;

	SCachedValue<void*> shader[eHWSC_Num];
	SCachedValue<ID3D11ShaderResourceView*>           shaderResourceView[eHWSC_Num][MAX_TMU];
	SCachedValue<ID3D11SamplerState*>                 samplerState[eHWSC_Num][MAX_TMU];
	SCachedValue<uint64>                              constantBuffer[eHWSC_Num][eConstantBufferShaderSlot_Count];

	std::array < std::bitset<ResourceSetMaxSrvCount>, eHWSC_Num > requiredSRVs;
	std::array < std::bitset<ResourceSetMaxUavCount>, eHWSC_Num > requiredUAVs;
	std::array < std::bitset<ResourceSetMaxSamplerCount>, eHWSC_Num > requiredSamplers;

	EShaderStage validShaderStages;
};

struct SCustomGraphicsState
{
	SCachedValue<ID3D11DepthStencilState*> depthStencilState;
	SCachedValue<ID3D11RasterizerState*>   rasterizerState;
	SCachedValue<ID3D11BlendState*>        blendState;
	SCachedValue<ID3D11InputLayout*>       inputLayout;
	SCachedValue<D3D11_PRIMITIVE_TOPOLOGY> topology;

	float depthConstBias;
	float depthSlopeBias;
	float depthBiasClamp;

	bool bRasterizerStateDirty;
	bool bDepthStencilStateDirty;
};

struct SCustomComputeState
{
	std::bitset<D3D11_PS_CS_UAV_REGISTER_COUNT> boundUAVs;
};

class CDeviceCommandListImpl : public CDeviceCommandListCommon<SSharedState, SCustomGraphicsState, SCustomComputeState>
{
public:
	CDeviceGraphicsCommandInterfaceImpl* GetGraphicsInterfaceImpl()
	{
		return reinterpret_cast<CDeviceGraphicsCommandInterfaceImpl*>(this);
	}

	CDeviceComputeCommandInterfaceImpl* GetComputeInterfaceImpl()
	{
		return reinterpret_cast<CDeviceComputeCommandInterfaceImpl*>(this);
	}

	CDeviceNvidiaCommandInterfaceImpl* GetNvidiaCommandInterfaceImpl();
	CDeviceCopyCommandInterfaceImpl* GetCopyInterfaceImpl()
	{
		return reinterpret_cast<CDeviceCopyCommandInterfaceImpl*>(this);
	}

	void SetProfilerMarker(const char* label);
	void BeginProfilerEvent(const char* label);
	void EndProfilerEvent(const char* label);

	// Helper functions for DX11
	NCryDX11::CCommandList* GetDX11CommandList() const { return m_sharedState.pCommandList; }

protected:
	void ClearStateImpl(bool bOutputMergerOnly) const;

	void ResetImpl();
	void LockToThreadImpl();
	void CloseImpl();

};

////////////////////////////////////////////////////////////////////////////

class CDeviceGraphicsCommandInterfaceImpl : public CDeviceCommandListImpl
{
protected:
	void PrepareUAVsForUseImpl(uint32 viewCount, CDeviceBuffer** pViews, bool bCompute) const {}
	void PrepareRenderPassForUseImpl(CDeviceRenderPass& renderPass) const {}
	void PrepareResourceForUseImpl(uint32 bindSlot, CTexture* pTexture, const ResourceViewHandle TextureView, ::EShaderStage srvUsage) const                              {}
	void PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources) const                                                                                {}
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const         {}
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages) const          {}
	void PrepareInlineShaderResourceForUseImpl(uint32 bindSlot, CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, EHWShaderClass shaderClass) const           {}
	void PrepareInlineShaderResourceForUseImpl(uint32 bindSlot, CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, EShaderStage shaderStages) const            {}
	void PrepareVertexBuffersForUseImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams) const                                          {}
	void PrepareIndexBufferForUseImpl(const CDeviceInputStream* indexStream) const                                                                                        {}
	void BeginResourceTransitionsImpl(uint32 numTextures, CTexture** pTextures, EResourceTransitionType type)                                                             {}

	void BeginRenderPassImpl(const CDeviceRenderPass& renderPass, const D3DRectangle& renderArea);
	void EndRenderPassImpl(const CDeviceRenderPass& renderPass) {}
	void SetViewportsImpl(uint32 vpCount, const D3DViewPort* pViewports);
	void SetScissorRectsImpl(uint32 rcCount, const D3DRectangle* pRects);
	void SetPipelineStateImpl(const CDeviceGraphicsPSO* pDevicePSO);
	void SetResourceLayoutImpl(const CDeviceResourceLayout* pResourceLayout) {}
	void SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages);
	void SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, EHWShaderClass shaderClass, ResourceViewHandle resourceViewID);
	void SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, ::EShaderStage shaderStages, ResourceViewHandle resourceViewID);
	void SetVertexBuffersImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams);
	void SetIndexBufferImpl(const CDeviceInputStream* indexStream); // NOTE: Take care with PSO strip cut/restart value and 32/16 bit indices
	void SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants) {}
	void SetStencilRefImpl(uint8 stencilRefValue);
	void SetDepthBiasImpl(float constBias, float slopeBias, float biasClamp);
	void SetDepthBoundsImpl(float fMin, float fMax);

	void DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation);
	void DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation);

	void ClearSurfaceImpl(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRects);
	void ClearSurfaceImpl(D3DDepthSurface* pView, int clearFlags, float depth, uint8 stencil, uint32 numRects, const D3D11_RECT* pRects);

	void DiscardContentsImpl(D3DResource* pResource, uint32 numRects, const D3D11_RECT* pRects);
	void DiscardContentsImpl(D3DBaseView* pView, uint32 numRects, const D3D11_RECT* pRects);

	void BeginOcclusionQueryImpl(D3DOcclusionQuery* pQuery);
	void EndOcclusionQueryImpl(D3DOcclusionQuery* pQuery);

protected:
	void SetResources_RequestedByShaderOnly(const CDeviceResourceSet* pResources);
	void SetResources_All(const CDeviceResourceSet* pResources);

	void ApplyDepthStencilState();
	void ApplyRasterizerState();
};

////////////////////////////////////////////////////////////////////////////

class CDeviceComputeCommandInterfaceImpl : public CDeviceCommandListImpl
{
protected:
	void PrepareUAVsForUseImpl(uint32 viewCount, CDeviceBuffer** pViews) const {}
	void PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources) const {}
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlots) const {}
	void PrepareInlineShaderResourceForUseImpl(uint32 bindSlot, CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot) const {}

	void SetPipelineStateImpl(const CDeviceComputePSO* pDevicePSO);
	void SetResourceLayoutImpl(const CDeviceResourceLayout* pResourceLayout) {}
	void SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot);
	void SetInlineShaderResourceImpl(uint32 bindSlot, const CDeviceBuffer* pBuffer, EShaderResourceShaderSlot shaderSlot, ResourceViewHandle resourceViewID);
	void SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants);

	void DispatchImpl(uint32 X, uint32 Y, uint32 Z);

	void ClearUAVImpl(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects);
	void ClearUAVImpl(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects);

	void DiscardUAVContentsImpl(D3DResource* pResource, uint32 numRects, const D3D11_RECT* pRects);
	void DiscardUAVContentsImpl(D3DBaseView* pView, uint32 numRects, const D3D11_RECT* pRects);
};

////////////////////////////////////////////////////////////////////////////

class CDeviceNvidiaCommandInterfaceImpl : public CDeviceCommandListImpl
{
protected:
	void SetModifiedWModeImpl(bool enabled, uint32_t numViewports, const float* pA, const float* pB);
};


////////////////////////////////////////////////////////////////////////////

class CDeviceCopyCommandInterfaceImpl : public CDeviceCommandListImpl
{
protected:
	void CopyImpl(D3DResource*    pSrc, D3DResource*    pDst);

	void CopyImpl(CDeviceBuffer*  pSrc, CDeviceBuffer*  pDst);
	void CopyImpl(D3DBuffer*      pSrc, D3DBuffer*      pDst);
	void CopyImpl(CDeviceTexture* pSrc, CDeviceTexture* pDst);
	void CopyImpl(CDeviceTexture* pSrc, D3DTexture*     pDst);
	void CopyImpl(D3DTexture*     pSrc, D3DTexture*     pDst);
	void CopyImpl(D3DTexture*     pSrc, CDeviceTexture* pDst);

	void CopyImpl(CDeviceBuffer*  pSrc, CDeviceBuffer*  pDst, const SResourceRegionMapping& regionMapping);
	void CopyImpl(D3DBuffer*      pSrc, D3DBuffer*      pDst, const SResourceRegionMapping& regionMapping);
	void CopyImpl(CDeviceTexture* pSrc, CDeviceTexture* pDst, const SResourceRegionMapping& regionMapping);
	void CopyImpl(D3DTexture*     pSrc, CDeviceTexture* pDst, const SResourceRegionMapping& regionMapping);

	void CopyImpl(const void* pSrc, CConstantBuffer* pDst, const SResourceMemoryAlignment& memoryLayout);
	void CopyImpl(const void* pSrc, CDeviceBuffer*   pDst, const SResourceMemoryAlignment& memoryLayout);
	void CopyImpl(const void* pSrc, CDeviceTexture*  pDst, const SResourceMemoryAlignment& memoryLayout);

	void CopyImpl(const void* pSrc, CConstantBuffer* pDst, const SResourceMemoryMapping& memoryMapping);
	void CopyImpl(const void* pSrc, CDeviceBuffer*   pDst, const SResourceMemoryMapping& memoryMapping);
	void CopyImpl(const void* pSrc, CDeviceTexture*  pDst, const SResourceMemoryMapping& memoryMapping);

	void CopyImpl(CDeviceBuffer*  pSrc, void* pDst, const SResourceMemoryAlignment& memoryLayout);
	void CopyImpl(CDeviceTexture* pSrc, void* pDst, const SResourceMemoryAlignment& memoryLayout);

	void CopyImpl(CDeviceBuffer*  pSrc, void* pDst, const SResourceMemoryMapping& memoryMapping);
	void CopyImpl(CDeviceTexture* pSrc, void* pDst, const SResourceMemoryMapping& memoryMapping);

private:
	// This is not supported on 11.x and are emulated to behave the same as 12
	void CopySubresourcesRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags, UINT NumSubresources);
};

