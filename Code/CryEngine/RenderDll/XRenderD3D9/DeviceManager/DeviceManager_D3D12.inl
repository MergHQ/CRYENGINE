// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef DEVMAN_USE_STAGING_POOL
	#error StagingPool is a requirement for DX12
#endif

//=============================================================================

HRESULT CDeviceManager::CreateFence(DeviceFenceHandle& query)
{
	HRESULT hr = S_FALSE;
	query = reinterpret_cast<DeviceFenceHandle>(new UINT64);
	hr = query ? S_OK : E_FAIL;
	if (!FAILED(hr))
	{
		IssueFence(query);
	}
	return hr;
}

HRESULT CDeviceManager::ReleaseFence(DeviceFenceHandle query)
{
	HRESULT hr = query ? S_OK : S_FALSE;
	delete reinterpret_cast<UINT64*>(query);
	return hr;
}

HRESULT CDeviceManager::IssueFence(DeviceFenceHandle query)
{
	HRESULT hr = query ? S_OK : E_FAIL;
	UINT64* handle = reinterpret_cast<UINT64*>(query);
	if (handle)
	{
		*handle = gcpRendD3D->GetDeviceContext().InsertFence();
	}
	return hr;
}

HRESULT CDeviceManager::SyncFence(DeviceFenceHandle query, bool block, bool flush)
{
	HRESULT hr = query ? S_FALSE : E_FAIL;
	UINT64* handle = reinterpret_cast<UINT64*>(query);
	if (handle)
	{
		hr = gcpRendD3D->GetDeviceContext().TestForFence(*handle);
		if (hr != S_OK)
		{
			CRY_ASSERT(!flush || !gRenDev->m_pRT || gRenDev->m_pRT->IsRenderThread()); // Can only flush from render thread

			// Test + Flush + No-Block is okay
			// Test + No-Flush + Block may not be okay, caution advised as it could deadlock
			if (flush)
			{
				hr = gcpRendD3D->GetDeviceContext().FlushToFence(*handle);
			}

			if (block)
			{
				hr = gcpRendD3D->GetDeviceContext().WaitForFence(*handle);
			}
		}
	}
	return hr;
}

//=============================================================================

HRESULT CDeviceManager::GetTimestampFrequency(void* pFrequency)
{
	HRESULT hr = S_OK;
	*((UINT64*)pFrequency) = gcpRendD3D->GetDeviceContext().GetTimestampFrequency();
	return hr;
}

HRESULT CDeviceManager::CreateTimestamp(DeviceTimestampHandle& query, bool bDisjointTest)
{
	HRESULT hr = S_FALSE;
	if (!bDisjointTest)
	{
		query = reinterpret_cast<DeviceFenceHandle>(new INT);
		hr = query ? S_OK : E_FAIL;
		if (!FAILED(hr))
		{
			*((INT*)query) = gcpRendD3D->GetDeviceContext().ReserveTimestamp();
		}
	}
	return hr;
}

HRESULT CDeviceManager::ReleaseTimestamp(DeviceTimestampHandle query)
{
	HRESULT hr = query ? S_OK : S_FALSE;
	delete reinterpret_cast<INT*>(query);
	return hr;
}

HRESULT CDeviceManager::IssueTimestamp(DeviceTimestampHandle query, bool begin)
{
	HRESULT hr = query ? S_OK : E_FAIL;
	INT* handle = reinterpret_cast<INT*>(query);
	if (handle)
	{
		gcpRendD3D->GetDeviceContext().InsertTimestamp(*handle, 0);
	}
	return hr;
}

HRESULT CDeviceManager::ResolveTimestamps(bool block, bool flush)
{
	HRESULT hr = S_OK;
	gcpRendD3D->GetDeviceContext().ResolveTimestamps();

	if (flush || block)
	{
		// This is forcing a sync of CPU and GPU timelines
		UINT64 fenceValue = gcpRendD3D->GetDeviceContext().InsertFence();

		if (flush)
		{
			CRY_ASSERT(!flush || !gRenDev->m_pRT || gRenDev->m_pRT->IsRenderThread()); // Can only flush from render thread

			hr = gcpRendD3D->GetDeviceContext().FlushToFence(fenceValue);
		}

		if (block)
		{
			hr = gcpRendD3D->GetDeviceContext().WaitForFence(fenceValue);
		}
	}

	return hr;
}

HRESULT CDeviceManager::SyncTimestamp(DeviceTimestampHandle query, void* pData, bool flush)
{
	HRESULT hr = query ? S_OK : E_FAIL;
	INT* handle = reinterpret_cast<INT*>(query);
	if (handle)
	{
		gcpRendD3D->GetDeviceContext().QueryTimestamp(*handle, (UINT64*)pData);
	}

	return hr;
}

HRESULT CDeviceManager::SyncTimestamps(INT offs, INT num, DeviceTimestampHandle* query, void* pData, bool flush)
{
	HRESULT hr = query ? S_OK : E_FAIL;
	INT* handle = reinterpret_cast<INT*>(query);
	if (handle)
	{
		gcpRendD3D->GetDeviceContext().QueryTimestamps(offs, offs + num, (UINT64*)pData);
	}
	return hr;
}

//=============================================================================

#if !(CRY_PLATFORM_DURANGO && BUFFER_ENABLE_DIRECT_ACCESS)
HRESULT CDeviceManager::InvalidateCpuCache(void* buffer_ptr, size_t size, size_t offset)
{
	return S_OK;
}

HRESULT CDeviceManager::InvalidateGpuCache(D3DBuffer* buffer, void* buffer_ptr, size_t size, size_t offset)
{
	return S_OK;
}

HRESULT CDeviceManager::InvalidateResourceGpuCache(D3DBuffer* buffer)
{
	return S_OK;
}

void CDeviceManager::InvalidateBuffer(D3DBuffer* buffer, void* base_ptr, size_t offset, size_t size, uint32 id)
{
}
#endif

//=============================================================================

D3DResource* CDeviceManager::AllocateNullResource(D3D11_RESOURCE_DIMENSION eType)
{
	D3DResource* pNullResource = m_NullResources[eType];

	if (pNullResource || (S_OK == gcpRendD3D->GetDevice_Unsynchronized().CreateNullResource(eType, &pNullResource)))
	{
		m_NullResources[eType] = pNullResource;
		m_NullResources[eType]->AddRef();
	}

	return pNullResource;
}

void CDeviceManager::ReleaseNullResource(D3DResource* pNullResource)
{
	if (pNullResource && gcpRendD3D->GetDevice_Unsynchronized().ReleaseNullResource(pNullResource))
	{
		pNullResource = nullptr;
	}
}

//=============================================================================

#ifdef DEVMAN_USE_STAGING_POOL
D3DResource* CDeviceManager::AllocateStagingResource(D3DResource* pForTex, bool bUpload)
{
	D3DResource* pStagingResource = nullptr;

	if (S_OK == gcpRendD3D->GetDevice_Unsynchronized().CreateStagingResource(pForTex, &pStagingResource, bUpload))
	{
		pStagingResource;
	}

	return pStagingResource;
}

void CDeviceManager::ReleaseStagingResource(D3DResource* pStagingTex)
{
	if (pStagingTex && gcpRendD3D->GetDevice_Unsynchronized().ReleaseStagingResource(pStagingTex))
	{
		pStagingTex = nullptr;
	}
}
#endif

//=============================================================================

int32 CDeviceTexture::Release()
{
	int32 nRef = Cleanup();

#if !defined(_RELEASE) && defined(_DEBUG)
	IF(nRef < 0, 0)
	__debugbreak();
#endif
	if (nRef == 0 && !m_bNoDelete)
	{
		delete this;
	}

	return nRef;
}

void CDeviceTexture::Unbind()
{
	for (uint32 i = 0; i < MAX_TMU; i++)
	{
		if (CTexture::s_TexStages[i].m_DevTexture == this)
		{
			CTexture::s_TexStages[i].m_DevTexture = NULL;

			ID3D11ShaderResourceView* RV = NULL;
			gcpRendD3D->GetDeviceContext().PSSetShaderResources(i, 1, &RV);
		}
	}
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes, StagingHook cbTransfer)
{
	D3D11_RESOURCE_DIMENSION eResourceDimension;
	m_pD3DTexture->GetType(&eResourceDimension);

	D3DResource* pStagingResource;
	void* pStagingMemory = m_pStagingMemory[0];
	if (!(pStagingResource = m_pStagingResource[0]))
	{
		pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, FALSE);
	}

	assert(pStagingResource);

	if (gcpRendD3D->GetDeviceContext_Unsynchronized().CopyStagingResource(pStagingResource, m_pD3DTexture, nSubRes, FALSE) == S_OK)
	{
		// Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
		// Map and Unmap must be called between CPU and GPU accesses to the same memory
		// address on some system architectures, when the page caching behavior is write-back.
		// Map and Unmap invalidate and flush the last level CPU cache on some ARM systems,
		// to marshal data between the CPU and GPU through memory addresses with write-back behavior.
		gcpRendD3D->GetDeviceContext_Unsynchronized().WaitStagingResource(pStagingResource);
		gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(pStagingResource, FALSE, &pStagingMemory);

		cbTransfer(pStagingMemory, 0, 0);

		gcpRendD3D->GetDeviceContext_Unsynchronized().UnmapStagingResource(pStagingResource, FALSE);
	}

	if (!(m_pStagingResource[0]))
	{
		gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
	}
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes)
{
	assert(m_pStagingResource[0]);

	gcpRendD3D->GetDeviceContext_Unsynchronized().CopyStagingResource(m_pStagingResource[0], m_pD3DTexture, nSubRes, FALSE);
}

void CDeviceTexture::UploadFromStagingResource(uint32 nSubRes, StagingHook cbTransfer)
{
	D3D11_RESOURCE_DIMENSION eResourceDimension;
	m_pD3DTexture->GetType(&eResourceDimension);

	D3DResource* pStagingResource;
	void* pStagingMemory = m_pStagingMemory[1];
	if (!(pStagingResource = m_pStagingResource[1]))
	{
		pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, TRUE);
	}

	assert(pStagingResource);

	// The first call to Map allocates a CPU virtual address range for the resource.
	// The last call to Unmap deallocates the CPU virtual address range.
	// Applications cannot rely on the address being consistent, unless Map is persistently nested.
	gcpRendD3D->GetDeviceContext_Unsynchronized().WaitStagingResource(pStagingResource);
	gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(pStagingResource, TRUE, &pStagingMemory);

	if (cbTransfer(pStagingMemory, 0, 0))
	{
		gcpRendD3D->GetDeviceContext_Unsynchronized().CopyStagingResource(pStagingResource, m_pD3DTexture, nSubRes, TRUE);
	}

	// Unmap also flushes the CPU cache, when necessary, so that GPU reads to this
	// address reflect any modifications made by the CPU.
	gcpRendD3D->GetDeviceContext_Unsynchronized().UnmapStagingResource(pStagingResource, TRUE);

	if (!(m_pStagingResource[1]))
	{
		gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
	}
}

void CDeviceTexture::UploadFromStagingResource(uint32 nSubRes)
{
	assert(m_pStagingResource[1]);

	gcpRendD3D->GetDeviceContext_Unsynchronized().CopyStagingResource(m_pStagingResource[1], m_pD3DTexture, nSubRes, TRUE);
}

void CDeviceTexture::AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer)
{
	// Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
	// Applications cannot rely on the address being consistent, unless Map is persistently nested.
	gcpRendD3D->GetDeviceContext_Unsynchronized().WaitStagingResource(m_pStagingResource[forUpload]);
	gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(m_pStagingResource[forUpload], forUpload, &m_pStagingMemory[forUpload]);

	cbTransfer(m_pStagingMemory[forUpload], 0, 0);

	gcpRendD3D->GetDeviceContext_Unsynchronized().UnmapStagingResource(m_pStagingResource[forUpload], forUpload);
}

//=============================================================================

#ifdef DEVMAN_USE_PINNING
void CDeviceTexture::WeakPin()
{
}

void CDeviceTexture::Pin()
{
}

void CDeviceTexture::Unpin()
{
}
#endif

//=============================================================================

HRESULT CDeviceManager::Create2DTexture(uint32 nWidth, uint32 nHeight, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI, int32 nESRAMOffset)
{
	HRESULT hr = S_OK;

	uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (nUsage & USAGE_DEPTH_STENCIL)
	{
		nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	else if (nUsage & USAGE_RENDER_TARGET)
	{
		nBindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if (nUsage & USAGE_UNORDERED_ACCESS)
	{
		nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}
	uint32 nMiscFlags = 0;
	if (nUsage & USAGE_AUTOGENMIPS)
	{
		nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	D3D11_TEXTURE2D_DESC Desc;
	ZeroStruct(Desc);
	Desc.Width = nWidth;
	Desc.Height = nHeight;
	Desc.MipLevels = nMips;
	Desc.Format = Format;
	Desc.ArraySize = nArraySize;
	Desc.BindFlags = nBindFlags;
	Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

	Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

	Desc.SampleDesc.Count = pTI ? pTI->m_nMSAASamples : 1;
	Desc.SampleDesc.Quality = pTI ? pTI->m_nMSAAQuality : 0;
	// AntonK: supported only by DX11 feature set
	// needs to be enabled for pure DX11 context
	//if(nUsage & USAGE_STREAMING)
	//nMiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
	//if (nUsage & USAGE_STREAMING)
	//    Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
	Desc.MiscFlags = nMiscFlags;

	if ((hr = Create2DTexture(Desc, cClearValue, ppDevTexture, pTI)) == S_OK)
	{
		CDeviceTexture* pDeviceTexture = *ppDevTexture;

		if (nUsage & USAGE_STAGE_ACCESS)
		{
			if (nUsage & USAGE_CPU_READ)
			{
				// Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				// Applications cannot rely on the address being consistent, unless Map is persistently nested.
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
				gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
			}
		}
	}

	return hr;
}

HRESULT CDeviceManager::CreateCubeTexture(uint32 nSize, uint32 nMips, uint32 nArraySize, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	HRESULT hr = S_OK;

	uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
	uint32 nMiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	if (nUsage & USAGE_DEPTH_STENCIL)
	{
		nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	else if (nUsage & USAGE_RENDER_TARGET)
	{
		nBindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if (nUsage & USAGE_AUTOGENMIPS)
	{
		nMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	D3D11_TEXTURE2D_DESC Desc;
	ZeroStruct(Desc);
	Desc.Width = nSize;
	Desc.Height = nSize;
	Desc.MipLevels = nMips;
	Desc.Format = Format;
	Desc.ArraySize = nArraySize * 6;
	Desc.BindFlags |= nBindFlags;
	Desc.CPUAccessFlags = (nUsage & USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
	Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.MiscFlags = nMiscFlags;
	// AntonK: supported only by DX11 feature set
	// needs to be enabled for pure DX11 context
	//if(nUsage & USAGE_STREAMING)
	//Desc.MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;

	if ((hr = CreateCubeTexture(Desc, cClearValue, ppDevTexture, pTI)) == S_OK)
	{
		CDeviceTexture* pDeviceTexture = *ppDevTexture;

		if (nUsage & USAGE_STAGE_ACCESS)
		{
			if (nUsage & USAGE_CPU_READ)
			{
				// Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				// Applications cannot rely on the address being consistent, unless Map is persistently nested.
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
				gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
			}
		}
	}

	return hr;
}

HRESULT CDeviceManager::CreateVolumeTexture(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nUsage, const ColorF& cClearValue, D3DFormat Format, D3DPOOL Pool, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	HRESULT hr = S_OK;

	uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
	if (nUsage & USAGE_DEPTH_STENCIL)
	{
		nBindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	else if (nUsage & USAGE_RENDER_TARGET)
	{
		nBindFlags |= D3D11_BIND_RENDER_TARGET;
	}
	if (nUsage & USAGE_UNORDERED_ACCESS)
	{
		nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	D3D11_TEXTURE3D_DESC Desc;
	ZeroStruct(Desc);
	Desc.Width = nWidth;
	Desc.Height = nHeight;
	Desc.Depth = nDepth;
	Desc.MipLevels = nMips;
	Desc.Format = (nUsage & USAGE_UAV_RWTEXTURE) ? CTexture::ConvertToTypelessFmt(Format) : Format;
	Desc.BindFlags = nBindFlags;
	Desc.CPUAccessFlags = 0;

	Desc.Usage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	Desc.Usage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : Desc.Usage;

	Desc.MiscFlags = 0;

	if ((hr = CreateVolumeTexture(Desc, cClearValue, ppDevTexture, pTI)) == S_OK)
	{
		CDeviceTexture* pDeviceTexture = *ppDevTexture;

		if (nUsage & USAGE_STAGE_ACCESS)
		{
			if (nUsage & USAGE_CPU_READ)
			{
				// Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map.
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				// Applications cannot rely on the address being consistent, unless Map is persistently nested.
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
				gcpRendD3D->GetDeviceContext_Unsynchronized().MapStagingResource(pDeviceTexture->m_pStagingResource[1], TRUE, &pDeviceTexture->m_pStagingMemory[1]);
			}
		}
	}

	return hr;
}

HRESULT CDeviceManager::Create2DTexture(const D3D11_TEXTURE2D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	CDeviceTexture* pDeviceTexture = 0;//*/ new CDeviceTexture();
	D3DTexture* pD3DTex = NULL;
	HRESULT hr = S_OK;

	D3D11_SUBRESOURCE_DATA* pSRD = NULL;
	D3D11_SUBRESOURCE_DATA SRD[20];
	if (pTI && pTI->m_pData)
	{
		pSRD = &SRD[0];
		for (UINT i = 0; i < Desc.MipLevels; i++)
		{
			SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
			SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
			SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
		}
	}

	if (Desc.BindFlags & (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS))
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTarget2D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
	}
	else
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTexture2D(&Desc, pSRD, &pD3DTex);
	}

	if (SUCCEEDED(hr) && pDeviceTexture == 0)
	{
		pDeviceTexture = new CDeviceTexture();
	}

	if (SUCCEEDED(hr) && pD3DTex)
	{
		pDeviceTexture->m_pD3DTexture = pD3DTex;
		if (!pDeviceTexture->m_nBaseAllocatedSize)
		{
			pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(Desc.Width, Desc.Height, 1, Desc.MipLevels, Desc.ArraySize, CTexture::TexFormatFromDeviceFormat(Desc.Format));
		}
	}
	else
	{
		SAFE_DELETE(pDeviceTexture);
	}

	*ppDevTexture = pDeviceTexture;

	return hr;
}

HRESULT CDeviceManager::CreateCubeTexture(const D3D11_TEXTURE2D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	CDeviceTexture* pDeviceTexture = 0;//*/ new CDeviceTexture();
	D3DCubeTexture* pD3DTex = NULL;
	HRESULT hr = S_OK;

	D3D11_SUBRESOURCE_DATA* pSRD = NULL;
	D3D11_SUBRESOURCE_DATA SRD[g_nD3D10MaxSupportedSubres];
	if (pTI && pTI->m_pData)
	{
		pSRD = &SRD[0];
		for (int j = 0; j < 6; j++)
		{
			for (int i = 0; i < Desc.MipLevels; i++)
			{
				int nSubresInd = j * Desc.MipLevels + i;
				SRD[nSubresInd].pSysMem = pTI->m_pData[nSubresInd].pSysMem;
				SRD[nSubresInd].SysMemPitch = pTI->m_pData[nSubresInd].SysMemPitch;
				SRD[nSubresInd].SysMemSlicePitch = pTI->m_pData[nSubresInd].SysMemSlicePitch;
			}
		}
	}

	if (Desc.BindFlags & (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS))
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTarget2D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
	}
	else
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTexture2D(&Desc, pSRD, &pD3DTex);
	}

	if (SUCCEEDED(hr) && pDeviceTexture == 0)
	{
		pDeviceTexture = new CDeviceTexture();
	}

	if (SUCCEEDED(hr) && pD3DTex)
	{
		pDeviceTexture->m_pD3DTexture = pD3DTex;
		if (!pDeviceTexture->m_nBaseAllocatedSize)
		{
			pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(Desc.Width, Desc.Height, 1, Desc.MipLevels, Desc.ArraySize, CTexture::TexFormatFromDeviceFormat(Desc.Format));
		}
	}
	else
	{
		SAFE_DELETE(pDeviceTexture);
	}

	*ppDevTexture = pDeviceTexture;

	return hr;
}

HRESULT CDeviceManager::CreateVolumeTexture(const D3D11_TEXTURE3D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	CDeviceTexture* pDeviceTexture = 0;//*/ new CDeviceTexture();
	D3DVolumeTexture* pD3DTex = NULL;
	HRESULT hr = S_OK;

	D3D11_SUBRESOURCE_DATA* pSRD = NULL;
	D3D11_SUBRESOURCE_DATA SRD[20] = { D3D11_SUBRESOURCE_DATA() };
	if (pTI && pTI->m_pData)
	{
		pSRD = &SRD[0];
		for (int i = 0; i < Desc.MipLevels; i++)
		{
			SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
			SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
			SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;
		}
	}

	if (Desc.BindFlags & (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS))
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTarget3D(&Desc, (const FLOAT*)&cClearValue, pSRD, &pD3DTex);
	}
	else
	{
		hr = gcpRendD3D->GetDevice_Unsynchronized().CreateTexture3D(&Desc, pSRD, &pD3DTex);
	}

	if (SUCCEEDED(hr) && pDeviceTexture == 0)
	{
		pDeviceTexture = new CDeviceTexture();
	}

	if (SUCCEEDED(hr) && pD3DTex)
	{
		pDeviceTexture->m_pD3DTexture = pD3DTex;
		if (!pDeviceTexture->m_nBaseAllocatedSize)
		{
			pDeviceTexture->m_nBaseAllocatedSize = CDeviceTexture::TextureDataSize(Desc.Width, Desc.Height, Desc.Depth, Desc.MipLevels, 1, CTexture::TexFormatFromDeviceFormat(Desc.Format));
		}
	}
	else
	{
		SAFE_DELETE(pDeviceTexture);
	}

	*ppDevTexture = pDeviceTexture;

	return hr;
}

HRESULT CDeviceManager::CreateBuffer(
  uint32 nSize
  , uint32 elemSize
  , int32 nUsage
  , int32 nBindFlags
  , D3DBuffer** ppBuff)
{
	FUNCTION_PROFILER(gEnv->pSystem, PROFILE_RENDERER);
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "CreateBuffer");
	HRESULT hr = S_OK;

#ifndef _RELEASE
	// ToDo verify that the usage and bindflags are correctly set (e.g certain
	// bit groups are mutually exclusive).
#endif

	D3D11_BUFFER_DESC BufDesc;
	ZeroStruct(BufDesc);

	BufDesc.ByteWidth = nSize * elemSize;
	int nD3DUsage = D3D11_USAGE_DEFAULT;
	nD3DUsage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : nD3DUsage;
	nD3DUsage = (nUsage & USAGE_IMMUTABLE) ? D3D11_USAGE_IMMUTABLE : nD3DUsage;
	nD3DUsage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : nD3DUsage;
	BufDesc.Usage = (D3D11_USAGE)nD3DUsage;

	if (BufDesc.Usage != D3D11_USAGE_STAGING)
	{
		switch (nBindFlags)
		{
		case BIND_VERTEX_BUFFER:
			BufDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
			break;
		case BIND_INDEX_BUFFER:
			BufDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
			break;
		case BIND_CONSTANT_BUFFER:
			BufDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
			break;

		case BIND_SHADER_RESOURCE:
		case BIND_UNORDERED_ACCESS:
			if (nBindFlags & BIND_SHADER_RESOURCE)
			{
				BufDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if (nBindFlags & BIND_UNORDERED_ACCESS)
			{
				BufDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}
			break;

		case BIND_STREAM_OUTPUT:
		case BIND_RENDER_TARGET:
		case BIND_DEPTH_STENCIL:
			CryFatalError("trying to create (currently) unsupported buffer type");
			break;

		default:
			CryFatalError("trying to create unknown buffer type");
			break;
		}
	}
	else
	{
		BufDesc.BindFlags = 0;
	}

	BufDesc.MiscFlags = 0;
	BufDesc.CPUAccessFlags = 0;
	if (BufDesc.Usage != D3D11_USAGE_DEFAULT && BufDesc.Usage != D3D11_USAGE_IMMUTABLE)
	{
		BufDesc.CPUAccessFlags = (nUsage & USAGE_CPU_WRITE) ? D3D11_CPU_ACCESS_WRITE : 0;
		BufDesc.CPUAccessFlags |= (nUsage & USAGE_CPU_READ) ? D3D11_CPU_ACCESS_READ : 0;
	}

	if (nBindFlags & BIND_UNORDERED_ACCESS)
	{
		BufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	}

	hr = gcpRendD3D->GetDevice_Unsynchronized().CreateBuffer(&BufDesc, NULL, ppBuff);
	CHECK_HRESULT(hr);
	return hr;
}
