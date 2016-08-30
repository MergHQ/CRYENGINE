// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(CRY_USE_DX12)
class CDeviceGraphicsCommandInterfaceImpl;
class CDeviceComputeCommandInterfaceImpl;
struct SEmpty {};

struct SSharedState
{
	DX12_PTR(NCryDX12::CCommandList) pCommandList;
};

struct SCustomGraphicsState
{
	SCachedValue<D3D12_PRIMITIVE_TOPOLOGY> primitiveTopology;
};

class CDeviceCommandListImpl : public CDeviceCommandListCommon<SSharedState, SCustomGraphicsState, SEmpty>
{
public:
	ILINE CDeviceGraphicsCommandInterfaceImpl* GetGraphicsInterfaceImpl()
	{
		return reinterpret_cast<CDeviceGraphicsCommandInterfaceImpl*>(this);
	}

	ILINE CDeviceComputeCommandInterfaceImpl* GetComputeInterfaceImpl()
	{
		return reinterpret_cast<CDeviceComputeCommandInterfaceImpl*>(this);
	}

	void SetProfilerMarker(const char* label);
	void BeginProfilerEvent(const char* label);
	void EndProfilerEvent(const char* label);

	// Helper functions for DX12
	NCryDX12::CCommandList* GetDX12CommandList() { return m_sharedState.pCommandList; }

protected:
	void ResetImpl();
	void LockToThreadImpl();
	void CloseImpl();
};

class CDeviceGraphicsCommandInterfaceImpl : public CDeviceCommandListImpl
{
public:
	void PrepareUAVsForUseImpl(uint32 viewCount, CGpuBuffer** pViews) const;
	void PrepareRenderTargetsForUseImpl(uint32 targetCount, CTexture** pTargets, SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews = nullptr) const;
	void PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const;
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass) const;
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages) const;
	void PrepareVertexBuffersForUseImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams) const;
	void PrepareIndexBufferForUseImpl(const CDeviceInputStream* indexStream) const;
	void BeginResourceTransitionsImpl(uint32 numTextures, CTexture** pTextures, EResourceTransitionType type);

	void SetRenderTargetsImpl(uint32 targetCount, CTexture* const* pTargets, const SDepthTexture* pDepthTarget, const SResourceView::KeyType* pRenderTargetViews = nullptr);
	void SetViewportsImpl(uint32 vpCount, const D3DViewPort* pViewports);
	void SetScissorRectsImpl(uint32 rcCount, const D3DRectangle* pRects);
	void SetPipelineStateImpl(const CDeviceGraphicsPSO* pDevicePSO);
	void SetResourceLayoutImpl(const CDeviceResourceLayout* pResourceLayout);
	void SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EHWShaderClass shaderClass);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot, EShaderStage shaderStages);
	void SetVertexBuffersImpl(uint32 numStreams, uint32 lastStreamSlot, const CDeviceInputStream* vertexStreams);
	void SetIndexBufferImpl(const CDeviceInputStream* indexStream);       // NOTE: Take care with PSO strip cut/restart value and 32/16 bit indices
	void SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants);
	void SetStencilRefImpl(uint8 stencilRefValue);

	void DrawImpl(uint32 VertexCountPerInstance, uint32 InstanceCount, uint32 StartVertexLocation, uint32 StartInstanceLocation);
	void DrawIndexedImpl(uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, int BaseVertexLocation, uint32 StartInstanceLocation);

	void ClearSurfaceImpl(D3DSurface* pView, const FLOAT Color[4], UINT NumRects, const D3D11_RECT* pRects);
};

class CDeviceComputeCommandInterfaceImpl : public CDeviceCommandListImpl
{
protected:
	void PrepareUAVsForUseImpl(uint32 viewCount, CGpuBuffer** pViews) const;
	void PrepareResourcesForUseImpl(uint32 bindSlot, CDeviceResourceSet* pResources, ::EShaderStage srvUsage) const;
	void PrepareInlineConstantBufferForUseImpl(uint32 bindSlot, CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlots) const;

	void SetPipelineStateImpl(const CDeviceComputePSO* pDevicePSO);
	void SetResourceLayoutImpl(const CDeviceResourceLayout* pResourceLayout);
	void SetResourcesImpl(uint32 bindSlot, const CDeviceResourceSet* pResources, ::EShaderStage srvUsage);
	void SetInlineConstantBufferImpl(uint32 bindSlot, const CConstantBuffer* pBuffer, EConstantBufferShaderSlot shaderSlot);
	void SetInlineConstantsImpl(uint32 bindSlot, uint32 constantCount, float* pConstants);

	void DispatchImpl(uint32 X, uint32 Y, uint32 Z);

	void ClearUAVImpl(D3DUAV* pView, const FLOAT Values[4], UINT NumRects, const D3D11_RECT* pRects);
	void ClearUAVImpl(D3DUAV* pView, const UINT Values[4], UINT NumRects, const D3D11_RECT* pRects);
};

#endif
