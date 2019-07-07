// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "DX11CommandScheduler.hpp"

namespace NCryDX11 {

class CDevice : public CRefCounted
{
public:
	static CDevice*                  Create(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL* pFeatureLevel);
	static CDevice*                  Create(D3DDevice* device, D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_9_1);

	ILINE D3DDevice*                 GetD3D11Device() const                 { return /*PassAddRef*/ (m_pD3DDevice); }
	ILINE D3DDeviceContext*          GetD3D11ImmediateDeviceContext() const { return /*PassAddRef*/ (m_pD3DImmediateDeviceContext); }

protected:
	CDevice(D3DDevice* d3dDevice, D3DDeviceContext* d3dDeviceContext, D3D_FEATURE_LEVEL featureLevel);
	~CDevice();

private:
	DX11_PTR(D3DDevice)        m_pD3DDevice;
	DX11_PTR(D3DDeviceContext) m_pD3DImmediateDeviceContext;
	
	D3D_FEATURE_LEVEL      m_featureLevel;

public:
	HRESULT                     CheckFeatureSupport(D3D11_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize) { return m_pD3DDevice->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize); }
	D3D_FEATURE_LEVEL           GetFeatureLevel() const { return m_featureLevel; }

	CCommandScheduler&          GetScheduler() { return m_Scheduler; }
	const CCommandScheduler&    GetScheduler() const { return m_Scheduler; }

	HRESULT STDMETHODCALLTYPE DuplicateCommittedResource(
		_In_ ID3D11Resource* pInputResource,
		_Out_ ID3D11Resource** ppOutputResource
#if DURANGO_USE_ESRAM
		,_In_ int32 nESRAMOffset = ~0,
		_In_ int32 nESRAMSize = 0
#endif
	) threadsafe;

	HRESULT STDMETHODCALLTYPE SubstituteUsedCommittedResource(
		_In_ const FVAL64& fenceValue,
		_Inout_ ID3D11Resource** ppSubstituteResource) threadsafe;

	HRESULT STDMETHODCALLTYPE CreateOrReuseStagingResource(
		_In_ ID3D11Resource* pInputResource,
		_Out_ ID3D11Resource** ppStagingResource,
		_In_ BOOL Upload) threadsafe;

	HRESULT STDMETHODCALLTYPE CreateOrReuseCommittedResource(
		_In_ const D3D11_HEAP_PROPERTIES* pHeapProperties,
		_In_ D3D11_BIND_FLAG HeapFlags,
		_In_ const D3D11_RESOURCE_DESC* pResourceDesc,
		_In_ REFIID riidResource,
		_COM_Outptr_opt_ void** ppvResource) threadsafe;

	HRESULT STDMETHODCALLTYPE CreateCommittedResource(
		_In_ const D3D11_HEAP_PROPERTIES* pHeapProperties,
		_In_ D3D11_BIND_FLAG HeapFlags,
		_In_ const D3D11_RESOURCE_DESC* pResourceDesc,
		_In_ REFIID riidResource,
		_COM_Outptr_opt_ void** ppvResource) threadsafe;

	void FlushReleaseHeap(UINT64 completedFenceValue, UINT64 pruneFenceValue) threadsafe;
	void ReleaseLater(const FVAL64& fenceValue, ID3D11Resource* pObject, bool bReusable = true) threadsafe;

	void FlushAndWaitForGPU();

private:
	// Objects that should be released when they are not in use anymore
	static CryCriticalSectionNonRecursive                  m_ReleaseHeapTheadSafeScope;
	static CryCriticalSectionNonRecursive                  m_RecycleHeapTheadSafeScope;

	struct ReleaseInfo
	{
		THash  hHash;
		UINT32 bFlags;
		UINT64 fenceValue;
	};

	struct RecycleInfo
	{
		ID3D11Resource* pObject;
		UINT64          fenceValue;
	};

	typedef std::unordered_map<ID3D11Resource*, ReleaseInfo>   TReleaseHeap;
	typedef std::unordered_map<THash, std::deque<RecycleInfo>> TRecycleHeap;

	TReleaseHeap m_ReleaseHeap;
	TRecycleHeap m_RecycleHeap;

	CCommandScheduler m_Scheduler;
};

}
