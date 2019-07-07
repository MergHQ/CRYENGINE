// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DX11Device.hpp"

namespace NCryDX11 {

//---------------------------------------------------------------------------------------------------------------------
CDevice* CDevice::Create(IDXGIAdapter* pAdapter, D3D_FEATURE_LEVEL* pFeatureLevel)
{
	abort();
	return nullptr;
}

CDevice* CDevice::Create(D3DDevice* device, D3D_FEATURE_LEVEL featureLevel)
{
	D3DDeviceContext* pDeviceContext = nullptr;
#if (CRY_RENDERER_DIRECT3D >= 111)
	device->GetImmediateContext1(&pDeviceContext);
#else
	device->GetImmediateContext(&pDeviceContext);
#endif

	CDevice* const pDevice = new CDevice(device, pDeviceContext, featureLevel);
	pDeviceContext->Release();
	return pDevice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------
CDevice::CDevice(D3DDevice* d3d11Device, D3DDeviceContext* d3dDeviceContext, D3D_FEATURE_LEVEL featureLevel)
	: m_pD3DDevice(d3d11Device)
	, m_pD3DImmediateDeviceContext(d3dDeviceContext)
	, m_featureLevel(featureLevel)
	// Must be constructed last as it relies on the deviceContext
	, m_Scheduler(this)
{
}

//---------------------------------------------------------------------------------------------------------------------
CDevice::~CDevice()
{
}

//---------------------------------------------------------------------------------------------------------------------
CryCriticalSectionNonRecursive CDevice::m_ReleaseHeapTheadSafeScope;
CryCriticalSectionNonRecursive CDevice::m_RecycleHeapTheadSafeScope;

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CDevice::CreateOrReuseStagingResource(
  _In_ ID3D11Resource* pInputResource,
  _Out_ ID3D11Resource** ppStagingResource,
  _In_ BOOL Upload) threadsafe
{
	struct
	{
		D3D11_BIND_FLAG       sHeapFlags;
		D3D11_HEAP_PROPERTIES sHeap;
		D3D11_RESOURCE_DESC   sResourceDesc;
	}
	clearableBlob;

	ZeroMemory(&clearableBlob, sizeof(clearableBlob));

	// NOTE: this is a staging resource, no flags permitting views are allowed
	clearableBlob.sHeapFlags = D3D11_BIND_FLAG(Upload ? D3D11_BIND_SHADER_RESOURCE : 0);
	clearableBlob.sHeap.Type = Upload ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_STAGING;
	clearableBlob.sHeap.CPUPageProperty = Upload ? D3D11_CPU_ACCESS_WRITE : D3D11_CPU_ACCESS_READ;
	clearableBlob.sResourceDesc.Dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	clearableBlob.sResourceDesc.Alignment = 0;

	pInputResource->GetType(&clearableBlob.sResourceDesc.Dimension);
	if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
	{
		D3D11_BUFFER_DESC sDesc;
		((ID3D11Buffer*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sResourceDesc.Width = sDesc.ByteWidth;
		clearableBlob.sResourceDesc.Height = 1;
		clearableBlob.sResourceDesc.DepthOrArraySize = 1;
		clearableBlob.sResourceDesc.MipLevels = 1;
		clearableBlob.sResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		clearableBlob.sResourceDesc.StructureByteSize = sDesc.StructureByteStride;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
	{
		D3D11_TEXTURE1D_DESC sDesc;
		((ID3D11Texture1D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = 1;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D11_TEXTURE2D_DESC sDesc;
		((ID3D11Texture2D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = sDesc.Height;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc = sDesc.SampleDesc;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
	{
		D3D11_TEXTURE3D_DESC sDesc;
		((ID3D11Texture3D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = sDesc.Height;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.Depth;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else
		return E_FAIL;

	// Remove flags incompatible with staging resources
	clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(clearableBlob.sResourceDesc.Flags & ~D3D11_RESOURCE_MISC_RESOURCE_CLAMP);

	ID3D11Resource* stagingResource = nullptr;
	HRESULT result = CreateOrReuseCommittedResource(
		&clearableBlob.sHeap,
		clearableBlob.sHeapFlags,
		&clearableBlob.sResourceDesc,
		IID_GFX_ARGS(&stagingResource));

	if (result == S_OK && stagingResource != nullptr)
	{
		*ppStagingResource = stagingResource;

		return S_OK;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CDevice::DuplicateCommittedResource(
	_In_ ID3D11Resource* pInputResource,
	_Out_ ID3D11Resource** ppOutputResource
#if DURANGO_USE_ESRAM
	,_In_ int32 nESRAMOffset,
	_In_ int32 nESRAMSize
#endif
)
{
	struct
	{
		D3D11_BIND_FLAG       sHeapFlags;
		D3D11_HEAP_PROPERTIES sHeap;
		D3D11_RESOURCE_DESC   sResourceDesc;
	}
	clearableBlob;

	ZeroMemory(&clearableBlob, sizeof(clearableBlob));

	clearableBlob.sResourceDesc.Dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	clearableBlob.sResourceDesc.Alignment = 0;

	pInputResource->GetType(&clearableBlob.sResourceDesc.Dimension);
	if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
	{
		D3D11_BUFFER_DESC sDesc;
		((ID3D11Buffer*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
		clearableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
		clearableBlob.sHeap.Type = sDesc.Usage;
		clearableBlob.sResourceDesc.Width = sDesc.ByteWidth;
		clearableBlob.sResourceDesc.Height = 1;
		clearableBlob.sResourceDesc.DepthOrArraySize = 1;
		clearableBlob.sResourceDesc.MipLevels = 1;
		clearableBlob.sResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		clearableBlob.sResourceDesc.StructureByteSize = sDesc.StructureByteStride;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
	{
		D3D11_TEXTURE1D_DESC sDesc;
		((ID3D11Texture1D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
		clearableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
		clearableBlob.sHeap.Type = sDesc.Usage;
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = 1;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D11_TEXTURE2D_DESC sDesc;
		((ID3D11Texture2D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
		clearableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
		clearableBlob.sHeap.Type = sDesc.Usage;
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = sDesc.Height;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc = sDesc.SampleDesc;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}
	else if (clearableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
	{
		D3D11_TEXTURE3D_DESC sDesc;
		((ID3D11Texture3D*)pInputResource)->GetDesc(&sDesc);
		clearableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
		clearableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
		clearableBlob.sHeap.Type = sDesc.Usage;
		clearableBlob.sResourceDesc.Width = sDesc.Width;
		clearableBlob.sResourceDesc.Height = sDesc.Height;
		clearableBlob.sResourceDesc.DepthOrArraySize = sDesc.Depth;
		clearableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
		clearableBlob.sResourceDesc.Format = sDesc.Format;
		clearableBlob.sResourceDesc.StructureByteSize = 0;
		clearableBlob.sResourceDesc.SampleDesc.Count = 1;
		clearableBlob.sResourceDesc.SampleDesc.Quality = 0;
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
	}

#if DURANGO_USE_ESRAM
	if (nESRAMOffset != SKIP_ESRAM)
	{
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(clearableBlob.sResourceDesc.Flags | D3D11X_RESOURCE_MISC_ESRAM_RESIDENT);
		clearableBlob.sHeap.Type = D3D11_USAGE_DEFAULT;
		clearableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(0);
	}
	else
		clearableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(clearableBlob.sResourceDesc.Flags & ~D3D11X_RESOURCE_MISC_ESRAM_RESIDENT);

	clearableBlob.sResourceDesc.ESRAMOffsetBytes = nESRAMOffset;
	clearableBlob.sResourceDesc.ESRAMUsageBytes = nESRAMSize;
#endif

	ID3D11Resource* outputResource = nullptr;
	HRESULT result = CreateOrReuseCommittedResource(
		&clearableBlob.sHeap,
		clearableBlob.sHeapFlags,
		&clearableBlob.sResourceDesc,
		IID_GFX_ARGS(&outputResource));

	if (result == S_OK && outputResource != nullptr)
	{
		*ppOutputResource = outputResource;

		::SetDebugName(outputResource, ::GetDebugName(pInputResource).c_str());
		return S_OK;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CDevice::SubstituteUsedCommittedResource(
	_In_ const FVAL64& fenceValue,
	_Inout_ ID3D11Resource** ppSubstituteResource) threadsafe
{
	const auto& fenceManager = GetScheduler().GetFenceManager();
	if (fenceManager.IsCompleted(fenceValue))
	{
		// Can continued to be used without substitution
		return S_FALSE;
	}

	ID3D11Resource* pDisposableResource = *ppSubstituteResource;
	HRESULT result = DuplicateCommittedResource(pDisposableResource, ppSubstituteResource);

	if (result == S_OK && *ppSubstituteResource != nullptr)
	{
		ReleaseLater(fenceValue, pDisposableResource, true);
		pDisposableResource->Release();

		return S_OK;
	}

	return result;
}

//---------------------------------------------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CDevice::CreateOrReuseCommittedResource(
	_In_ const D3D11_HEAP_PROPERTIES* pHeapProperties,
	_In_ D3D11_BIND_FLAG HeapFlags,
	_In_ const D3D11_RESOURCE_DESC* pResourceDesc,
	_In_ REFIID riidResource,
	_COM_Outptr_opt_ void** ppvResource) threadsafe
{
	struct
	{
		D3D11_BIND_FLAG       sHeapFlags;
		D3D11_HEAP_PROPERTIES sHeapProperties;
		D3D11_RESOURCE_DESC   sResourceDesc;
	}
	hashableBlob;

	ZeroMemory(&hashableBlob, sizeof(hashableBlob));

	hashableBlob.sHeapFlags = HeapFlags;
	hashableBlob.sHeapProperties = *pHeapProperties;
	hashableBlob.sResourceDesc = *pResourceDesc;
	hashableBlob.sResourceDesc.Alignment = 0; // alignment is intrinsic
	
	// Check type size assumptions about this padding clearing
	static_assert(sizeof(UINT64)      == 8, "UINT64 is not 8 bytes");
	static_assert(sizeof(UINT)        == 4, "UINT is not 4 bytes");
	static_assert(sizeof(UINT16)      == 2, "UINT16 is not 2 bytes");

	// There is no guarantee that enum backing types are 4 bytes
	static_assert(sizeof(D3D11_USAGE)              == 4, "D3D11_USAGE backing type is not 4 bytes");
	static_assert(sizeof(D3D11_CPU_ACCESS_FLAG)    == 4, "D3D11_CPU_ACCESS_FLAG backing type is not 4 bytes");
	static_assert(sizeof(D3D11_RESOURCE_DIMENSION) == 4, "D3D11_RESOURCE_DIMENSION backing type is not 4 bytes");
	static_assert(sizeof(DXGI_FORMAT)              == 4, "DXGI_FORMAT backing type is not 4 bytes");
	static_assert(sizeof(D3D11_RESOURCE_MISC_FLAG) == 4, "D3D11_RESOURCE_MISC_FLAG backing type is not 4 bytes");
	static_assert(sizeof(DXGI_SAMPLE_DESC)         == 8, "DXGI_SAMPLE_DESC is not 8 bytes");
	
	// Clear spaces from struct padding
	void* ptr1 = ((char*)&hashableBlob.sHeapProperties.CPUPageProperty) + sizeof(hashableBlob.sHeapProperties.CPUPageProperty);
	ZeroMemory(ptr1, sizeof(hashableBlob.sHeapProperties) - offsetof(D3D11_HEAP_PROPERTIES, CPUPageProperty) - sizeof(hashableBlob.sHeapProperties.CPUPageProperty));

	void* ptr2 = ((char*)&hashableBlob.sResourceDesc.Dimension) + sizeof(hashableBlob.sResourceDesc.Dimension);
	ZeroMemory(ptr2, offsetof(D3D11_RESOURCE_DESC, Alignment) - sizeof(hashableBlob.sResourceDesc.Dimension));
#if DURANGO_USE_ESRAM
	void* ptr3 = ((char*)&hashableBlob.sResourceDesc.ESRAMUsageBytes) + sizeof(hashableBlob.sResourceDesc.ESRAMUsageBytes);
	ZeroMemory(ptr3, sizeof(hashableBlob.sResourceDesc) - offsetof(D3D11_RESOURCE_DESC, ESRAMUsageBytes) - sizeof(hashableBlob.sResourceDesc.ESRAMUsageBytes));
#else
	void* ptr3 = ((char*)&hashableBlob.sResourceDesc.Flags) + sizeof(hashableBlob.sResourceDesc.Flags);
	ZeroMemory(ptr3, sizeof(hashableBlob.sResourceDesc) - offsetof(D3D11_RESOURCE_DESC, Flags) - sizeof(hashableBlob.sResourceDesc.Flags));
#endif

	THash hHash = ComputeSmallHash<sizeof(hashableBlob)>(&hashableBlob);

	{
		CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_RecycleHeapTheadSafeScope);

		// Best-case O(1) lookup
		TRecycleHeap::iterator result = m_RecycleHeap.find(hHash);
		if (result != m_RecycleHeap.end())
		{
			if (ppvResource)
			{
				// Guaranteed O(1) lookup
				*ppvResource = result->second.front().pObject;
				ClearDebugName(result->second.front().pObject);

				result->second.pop_front();
				if (!result->second.size())
					m_RecycleHeap.erase(result);
			}

			return S_OK;
		}
	}

	return CreateCommittedResource(pHeapProperties, HeapFlags, pResourceDesc, riidResource, ppvResource);
}

HRESULT STDMETHODCALLTYPE CDevice::CreateCommittedResource(
	_In_ const D3D11_HEAP_PROPERTIES* pHeapProperties,
	_In_ D3D11_BIND_FLAG HeapFlags,
	_In_ const D3D11_RESOURCE_DESC* pResourceDesc,
	_In_ REFIID riidResource,
	_COM_Outptr_opt_ void** ppvResource) threadsafe
{
	if (pResourceDesc->Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
	{
		D3D11_BUFFER_DESC sDesc;

		sDesc.ByteWidth = UINT(pResourceDesc->Width);
		sDesc.Usage = pHeapProperties->Type;
		sDesc.BindFlags = HeapFlags;
		sDesc.CPUAccessFlags = pHeapProperties->CPUPageProperty;
		sDesc.MiscFlags = pResourceDesc->Flags;
		sDesc.StructureByteStride = pResourceDesc->StructureByteSize;
#if DURANGO_USE_ESRAM
		sDesc.ESRAMOffsetBytes = pResourceDesc->ESRAMOffsetBytes;
		sDesc.ESRAMUsageBytes = pResourceDesc->ESRAMUsageBytes;
#endif

		return GetD3D11Device()->CreateBuffer(&sDesc, nullptr, (ID3D11Buffer**)ppvResource);
	}
	else if (pResourceDesc->Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
	{
		D3D11_TEXTURE1D_DESC sDesc;

		sDesc.Width = UINT(pResourceDesc->Width);
		sDesc.MipLevels = pResourceDesc->MipLevels;
		sDesc.ArraySize = pResourceDesc->DepthOrArraySize;
		sDesc.Format = pResourceDesc->Format;
		sDesc.Usage = pHeapProperties->Type;
		sDesc.BindFlags = HeapFlags;
		sDesc.CPUAccessFlags = pHeapProperties->CPUPageProperty;
		sDesc.MiscFlags = pResourceDesc->Flags;
#if DURANGO_USE_ESRAM
		sDesc.ESRAMOffsetBytes = pResourceDesc->ESRAMOffsetBytes;
		sDesc.ESRAMUsageBytes = pResourceDesc->ESRAMUsageBytes;
#endif

		return GetD3D11Device()->CreateTexture1D(&sDesc, nullptr, (ID3D11Texture1D**)ppvResource);
	}
	else if (pResourceDesc->Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D11_TEXTURE2D_DESC sDesc;

		sDesc.Width = UINT(pResourceDesc->Width);
		sDesc.Height = UINT(pResourceDesc->Height);
		sDesc.MipLevels = pResourceDesc->MipLevels;
		sDesc.ArraySize = pResourceDesc->DepthOrArraySize;
		sDesc.Format = pResourceDesc->Format;
		sDesc.SampleDesc = pResourceDesc->SampleDesc;
		sDesc.Usage = pHeapProperties->Type;
		sDesc.BindFlags = HeapFlags;
		sDesc.CPUAccessFlags = pHeapProperties->CPUPageProperty;
		sDesc.MiscFlags = pResourceDesc->Flags;
#if DURANGO_USE_ESRAM
		sDesc.ESRAMOffsetBytes = pResourceDesc->ESRAMOffsetBytes;
		sDesc.ESRAMUsageBytes = pResourceDesc->ESRAMUsageBytes;
#endif

		return GetD3D11Device()->CreateTexture2D(&sDesc, nullptr, (ID3D11Texture2D**)ppvResource);
	}
	else if (pResourceDesc->Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
	{
		D3D11_TEXTURE3D_DESC sDesc;

		sDesc.Width = UINT(pResourceDesc->Width);
		sDesc.Height = UINT(pResourceDesc->Height);
		sDesc.MipLevels = pResourceDesc->MipLevels;
		sDesc.Depth = pResourceDesc->DepthOrArraySize;
		sDesc.Format = pResourceDesc->Format;
		sDesc.Usage = pHeapProperties->Type;
		sDesc.BindFlags = HeapFlags;
		sDesc.CPUAccessFlags = pHeapProperties->CPUPageProperty;
		sDesc.MiscFlags = pResourceDesc->Flags;
#if DURANGO_USE_ESRAM
		sDesc.ESRAMOffsetBytes = pResourceDesc->ESRAMOffsetBytes;
		sDesc.ESRAMUsageBytes = pResourceDesc->ESRAMUsageBytes;
#endif

		return GetD3D11Device()->CreateTexture3D(&sDesc, nullptr, (ID3D11Texture3D**)ppvResource);
	}

	return E_FAIL;
}

//---------------------------------------------------------------------------------------------------------------------
void CDevice::FlushReleaseHeap(const UINT64 completedFenceValue, const UINT64 pruneFenceValue) threadsafe
{
	uint32 releases = 0;
	uint32 recyclations = 0;
	uint32 evictions = 0;

	{
		CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_ReleaseHeapTheadSafeScope);

		TReleaseHeap::iterator it, nx;
		for (it = m_ReleaseHeap.begin(); it != m_ReleaseHeap.end(); it = nx)
		{
			nx = it;
			++nx;

			if (((it->second.fenceValue) <= completedFenceValue))
			{
				if (((it->second.fenceValue) > pruneFenceValue) && (it->second.bFlags & 1))
				{
					CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_RecycleHeapTheadSafeScope);

					RecycleInfo recycleInfo;

					recycleInfo.pObject = it->first;
					recycleInfo.fenceValue = it->second.fenceValue;

					// TODO: could be accumulated to a local list and batch-merged in the next code-block to prevent the locking
					auto& sorted = m_RecycleHeap[it->second.hHash];
#if OUT_OF_ODER_RELEASE_LATER
					if (sorted.size())
					{
						auto insertionpoint = sorted.begin();
						while (insertionpoint->fenceValue > recycleInfo.fenceValue)
							++insertionpoint;
						sorted.insert(insertionpoint, std::move(recycleInfo));
					}
					else
#endif
						sorted.push_front(std::move(recycleInfo));

					recyclations++;
				}
				else
				{
					CRY_DX11_VERIFY(it->first->Release() == 0, "Invalid ref-count of D3D11 resource %s, memory will leak!", GetDebugName(it->first).c_str());

					releases++;
				}

				m_ReleaseHeap.erase(it);
			}
		}
	}
	
	//	if (0)
	{
		CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_RecycleHeapTheadSafeScope);

		TRecycleHeap::iterator it, nx;
		for (it = m_RecycleHeap.begin(); it != m_RecycleHeap.end(); it = nx)
		{
			nx = it;
			++nx;

			while (((it->second.back().fenceValue) <= pruneFenceValue))
			{
				// NOTE: some of the objects are referenced by multiple classes,
				// so even when the CResource is destructed and the d3d resource
				// given up for release, they can continue being in use
				// This means the ref-count here doesn't necessarily need to be 0
				CRY_DX11_VERIFY(it->second.back().pObject->Release() == 0, "Invalid ref-count of D3D11 resource %s, memory will leak!", GetDebugName(it->second.back().pObject).c_str());

				it->second.pop_back();
				evictions++;

				if (!it->second.size())
				{
					m_RecycleHeap.erase(it);
					break;
				}
			}
		}
	}

	// Output debug information
	if (CRendererCVars::CV_r_ShowVideoMemoryStats)
	{
		float fontSize = 1.7f;
		float fontSizeSmall = 1.2f;
		int rowHeight = 35;
		int rowHeightSmall = 13;
		int rowPos = 25;
		int rowMax = 25;
		float colPos = 20;
		size_t recycleSize = 0;
		size_t recycleNums = 0;

		const auto& fenceManager = GetScheduler().GetFenceManager();

		IRenderAuxText::Draw2dLabel(colPos, float(rowPos += rowHeight), 2.0f, Col_Blue, false, "GPU Resource Heap Debug");
		IRenderAuxText::Draw2dLabel(colPos, float(rowPos += rowHeight), fontSize, Col_Blue, false, "Fences: Current %lli, Submitted %lli, Completed %lli",
			fenceManager.GetCurrentValue(), fenceManager.GetSubmittedValue(), fenceManager.GetLastCompletedFenceValue());
		IRenderAuxText::Draw2dLabel(colPos, float(rowPos += rowHeight), fontSize, Col_Blue, false, "Movements: Releases %lli, Recyclations %lli, Evictions %lli",
			releases, recyclations, evictions);

		IRenderAuxText::Draw2dLabel(colPos, float(rowPos += rowHeight), 2.0f, Col_Yellow, false, "Recycle-heap:");

		rowPos += (rowHeight - rowHeightSmall);
		int rowReset = rowPos;
		float colReset = colPos;

		CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_RecycleHeapTheadSafeScope);
		TRecycleHeap::iterator it, nx;
		for (it = m_RecycleHeap.begin(); it != m_RecycleHeap.end(); it = nx)
		{
			nx = it;
			++nx;

			int counter = it->second.size();
			ID3D11Resource* pInputResource = it->second.front().pObject;

			D3D11_RESOURCE_DIMENSION Dimension;
			pInputResource->GetType(&Dimension);
			if (Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
			{
				D3D11_BUFFER_DESC sDesc;
				((ID3D11Buffer*)pInputResource)->GetDesc(&sDesc);

				uint32 size = sDesc.ByteWidth;
				recycleSize += counter * size;
				recycleNums += counter;

				IRenderAuxText::Draw2dLabel /* Ex */(colPos, float(rowPos += rowHeightSmall), fontSizeSmall, Col_Yellow, false /* eDrawText_Monospace */,
					"%2x: BUF %8d bytes [%1d %3x %1x %5x], %3d", counter, size,
					sDesc.Usage, sDesc.BindFlags, sDesc.CPUAccessFlags >> 16, sDesc.MiscFlags,
					sDesc.StructureByteStride);
			}
			else if (Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
			{
				D3D11_TEXTURE1D_DESC sDesc;
				((ID3D11Texture1D*)pInputResource)->GetDesc(&sDesc);

				uint32 size = CTexture::TextureDataSize(sDesc.Width, 1, 1, sDesc.MipLevels, sDesc.ArraySize, DeviceFormats::ConvertToTexFormat(sDesc.Format), eTM_Optimal);
				recycleSize += counter * size;
				recycleNums += counter;

				IRenderAuxText::Draw2dLabel /* Ex */(colPos, float(rowPos += rowHeightSmall), fontSizeSmall, Col_Yellow, false /* eDrawText_Monospace */,
					"%2x: T1D %8d bytes [%1d %3x %1x %5x], %d[%d]*%d", counter, size,
					sDesc.Usage, sDesc.BindFlags, sDesc.CPUAccessFlags >> 16, sDesc.MiscFlags,
					sDesc.Width, sDesc.ArraySize, sDesc.MipLevels);
			}
			else if (Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
			{
				D3D11_TEXTURE2D_DESC sDesc;
				((ID3D11Texture2D*)pInputResource)->GetDesc(&sDesc);

				uint32 size = CTexture::TextureDataSize(sDesc.Width, sDesc.Height, 1, sDesc.MipLevels, sDesc.ArraySize, DeviceFormats::ConvertToTexFormat(sDesc.Format), eTM_Optimal);
				recycleSize += counter * size;
				recycleNums += counter;

				IRenderAuxText::Draw2dLabel /* Ex */(colPos, float(rowPos += rowHeightSmall), fontSizeSmall, Col_Yellow, false /* eDrawText_Monospace */,
					"%2x: T2D %8d bytes [%1d %3x %1x %5x], %dx%d[%d]*%d", counter, size,
					sDesc.Usage, sDesc.BindFlags, sDesc.CPUAccessFlags >> 16, sDesc.MiscFlags,
					sDesc.Width, sDesc.Height, sDesc.ArraySize, sDesc.MipLevels);
			}
			else if (Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
			{
				D3D11_TEXTURE3D_DESC sDesc;
				((ID3D11Texture3D*)pInputResource)->GetDesc(&sDesc);

				uint32 size = CTexture::TextureDataSize(sDesc.Width, sDesc.Height, sDesc.Depth, sDesc.MipLevels, 1, DeviceFormats::ConvertToTexFormat(sDesc.Format), eTM_Optimal);
				recycleSize += counter * size;
				recycleNums += counter;

				IRenderAuxText::Draw2dLabel /* Ex */(colPos, float(rowPos += rowHeightSmall), fontSizeSmall, Col_Yellow, false /* eDrawText_Monospace */,
					"%2x: T3D %8d bytes [%1d %3x %1x %5x], %dx%dx%d*%d", counter, size,
					sDesc.Usage, sDesc.BindFlags, sDesc.CPUAccessFlags >> 16, sDesc.MiscFlags,
					sDesc.Width, sDesc.Height, sDesc.Depth, sDesc.MipLevels);
			}

			if (rowPos >= (400 - rowHeightSmall))
			{
				rowMax = std::max(rowMax, rowPos);
				rowPos = rowReset;
				colPos += 300;
			}
		}

		rowPos = std::max(rowMax, rowPos);
		colPos = colReset;

		IRenderAuxText::Draw2dLabel(colPos + 150, float(rowReset - (rowHeight - rowHeightSmall)), 2.0f, Col_Orange, false, "%d bytes, %d elements", recycleSize, recycleNums);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void CDevice::ReleaseLater(const FVAL64& fenceValue, ID3D11Resource* pObject, bool bReusable) threadsafe
{
	if (pObject)
	{
		struct
		{
			D3D11_BIND_FLAG       sHeapFlags;
			D3D11_HEAP_PROPERTIES sHeap;
			D3D11_RESOURCE_DESC   sResourceDesc;
		}
		hashableBlob;

		ZeroMemory(&hashableBlob, sizeof(hashableBlob));

		hashableBlob.sResourceDesc.Dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
		hashableBlob.sResourceDesc.Alignment = 0;

		pObject->GetType(&hashableBlob.sResourceDesc.Dimension);
		if (hashableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
		{
			D3D11_BUFFER_DESC sDesc;
			((ID3D11Buffer*)pObject)->GetDesc(&sDesc);
			hashableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
			hashableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
			hashableBlob.sHeap.Type = sDesc.Usage;
			hashableBlob.sResourceDesc.Width = sDesc.ByteWidth;
			hashableBlob.sResourceDesc.Height = 1;
			hashableBlob.sResourceDesc.DepthOrArraySize = 1;
			hashableBlob.sResourceDesc.MipLevels = 1;
			hashableBlob.sResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			hashableBlob.sResourceDesc.StructureByteSize = sDesc.StructureByteStride;
			hashableBlob.sResourceDesc.SampleDesc.Count = 1;
			hashableBlob.sResourceDesc.SampleDesc.Quality = 0;
			hashableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
		}
		else if (hashableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
		{
			D3D11_TEXTURE1D_DESC sDesc;
			((ID3D11Texture1D*)pObject)->GetDesc(&sDesc);
			hashableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
			hashableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
			hashableBlob.sHeap.Type = sDesc.Usage;
			hashableBlob.sResourceDesc.Width = sDesc.Width;
			hashableBlob.sResourceDesc.Height = 1;
			hashableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
			hashableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
			hashableBlob.sResourceDesc.Format = sDesc.Format;
			hashableBlob.sResourceDesc.StructureByteSize = 0;
			hashableBlob.sResourceDesc.SampleDesc.Count = 1;
			hashableBlob.sResourceDesc.SampleDesc.Quality = 0;
			hashableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
		}
		else if (hashableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		{
			D3D11_TEXTURE2D_DESC sDesc;
			((ID3D11Texture2D*)pObject)->GetDesc(&sDesc);
			hashableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
			hashableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
			hashableBlob.sHeap.Type = sDesc.Usage;
			hashableBlob.sResourceDesc.Width = sDesc.Width;
			hashableBlob.sResourceDesc.Height = sDesc.Height;
			hashableBlob.sResourceDesc.DepthOrArraySize = sDesc.ArraySize;
			hashableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
			hashableBlob.sResourceDesc.Format = sDesc.Format;
			hashableBlob.sResourceDesc.StructureByteSize = 0;
			hashableBlob.sResourceDesc.SampleDesc = sDesc.SampleDesc;
			hashableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
		}
		else if (hashableBlob.sResourceDesc.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
		{
			D3D11_TEXTURE3D_DESC sDesc;
			((ID3D11Texture3D*)pObject)->GetDesc(&sDesc);
			hashableBlob.sHeapFlags = D3D11_BIND_FLAG(sDesc.BindFlags);
			hashableBlob.sHeap.CPUPageProperty = D3D11_CPU_ACCESS_FLAG(sDesc.CPUAccessFlags);
			hashableBlob.sHeap.Type = sDesc.Usage;
			hashableBlob.sResourceDesc.Width = sDesc.Width;
			hashableBlob.sResourceDesc.Height = sDesc.Height;
			hashableBlob.sResourceDesc.DepthOrArraySize = sDesc.Depth;
			hashableBlob.sResourceDesc.MipLevels = sDesc.MipLevels;
			hashableBlob.sResourceDesc.Format = sDesc.Format;
			hashableBlob.sResourceDesc.StructureByteSize = 0;
			hashableBlob.sResourceDesc.SampleDesc.Count = 1;
			hashableBlob.sResourceDesc.SampleDesc.Quality = 0;
			hashableBlob.sResourceDesc.Flags = D3D11_RESOURCE_MISC_FLAG(sDesc.MiscFlags);
		}

		THash hHash = ComputeSmallHash<sizeof(hashableBlob)>(&hashableBlob);
		const bool isGPUOnly =
			hashableBlob.sHeap.Type == D3D11_USAGE_DEFAULT ||
			hashableBlob.sHeap.Type == D3D11_USAGE_IMMUTABLE;

		const FVAL64 completedFenceValue = m_Scheduler.GetFenceManager().GetLastCompletedFenceValue();
		const UINT64 fenceValueWithPrunningDelay = std::max(fenceValue, completedFenceValue);
		
		const bool isUnused =
			SmallerEqualFenceValues(fenceValueWithPrunningDelay, completedFenceValue);

		// GPU-only resources can't race with itself when they are managed by ref-counts/pools
		// CPU-write resources can be recycled immediately if they have been used up already
		if ((isGPUOnly | isUnused) & bReusable)
		{
			CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_RecycleHeapTheadSafeScope);

			RecycleInfo recycleInfo;

			recycleInfo.pObject = pObject;
			recycleInfo.fenceValue = fenceValue;

			auto& sorted = m_RecycleHeap[hHash];
#if OUT_OF_ODER_RELEASE_LATER
			if (sorted.size())
			{
				auto insertionpoint = sorted.begin();
				while (insertionpoint->fenceValue > recycleInfo.fenceValue)
					++insertionpoint;
				sorted.insert(insertionpoint, std::move(recycleInfo));
			}
			else
#endif
				sorted.push_front(std::move(recycleInfo));

			// Only count if insertion happened
			{
				pObject->AddRef();
			}
		}
		else if (!isUnused)
		{
			CryAutoLock<CryCriticalSectionNonRecursive> lThreadSafeScope(m_ReleaseHeapTheadSafeScope);

			ReleaseInfo releaseInfo;

			releaseInfo.hHash = hHash;
			releaseInfo.bFlags = bReusable ? 1 : 0;
			releaseInfo.fenceValue = fenceValue;

			std::pair<TReleaseHeap::iterator, bool> result = m_ReleaseHeap.emplace(pObject, std::move(releaseInfo));

			// Only count if insertion happened
			if (result.second)
			{
				pObject->AddRef();
			}
		}
	
#if 0	// NOTE: Use the code-fragment to detect resources without names
		if (GetDebugName(pObject).empty())
			__debugbreak();
#endif
	}
}

//---------------------------------------------------------------------------------------------------------------------
void CDevice::FlushAndWaitForGPU()
{
	// Submit pending command-lists in case there are left-overs, make sure it's flushed to and executed on the hardware
	m_Scheduler.Flush(true);
}

}
