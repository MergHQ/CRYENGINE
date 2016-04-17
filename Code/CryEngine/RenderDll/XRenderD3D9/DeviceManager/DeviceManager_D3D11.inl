// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if !CRY_USE_DX12 && !CRY_PLATFORM_ORBIS && !(CRY_PLATFORM_DURANGO && BUFFER_ENABLE_DIRECT_ACCESS)
HRESULT CDeviceManager::CreateFence(DeviceFenceHandle& query)
{
	HRESULT hr = S_FALSE;
	D3D11_QUERY_DESC QDesc;
	QDesc.Query = D3D11_QUERY_EVENT;
	QDesc.MiscFlags = 0;
	D3DQuery* d3d_query;
	hr = gcpRendD3D->GetDevice().CreateQuery(&QDesc, &d3d_query);
	if (CHECK_HRESULT(hr))
	{
		query = reinterpret_cast<DeviceFenceHandle>(d3d_query);
	}
	if (!FAILED(hr))
	{
		IssueFence(query);
	}
	return hr;
}

HRESULT CDeviceManager::ReleaseFence(DeviceFenceHandle query)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	SAFE_RELEASE(d3d_query);
	hr = S_OK;
	return hr;
}

HRESULT CDeviceManager::IssueFence(DeviceFenceHandle query)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	if (d3d_query)
	{
		gcpRendD3D->GetDeviceContext().End(d3d_query);
		hr = S_OK;
	}
	return hr;
}

HRESULT CDeviceManager::SyncFence(DeviceFenceHandle query, bool block, bool flush)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	if (d3d_query)
	{
		CRY_ASSERT(!flush || !gRenDev->m_pRT || gRenDev->m_pRT->IsRenderThread()); // Can only flush from render thread

		BOOL bQuery = false;
		do
		{
			hr = gcpRendD3D->GetDeviceContext().GetData(d3d_query, (void*)&bQuery, sizeof(BOOL), flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
	#if !defined(_RELEASE)
			if (hr != S_OK && hr != S_FALSE)
			{
				CHECK_HRESULT(hr);
			}
	#endif
		}
		while (block && hr != S_OK);
	}
	return hr;
}
#endif

//=============================================================================

#if !CRY_USE_DX12
HRESULT CDeviceManager::GetTimestampFrequency(void* pFrequency)
{
	HRESULT hr = S_FALSE;
	DeviceTimestampHandle freq;
	if (CreateTimestamp(freq, true) == S_OK)
	{
		if (IssueTimestamp(freq, true) == S_OK)
		{
			if (IssueTimestamp(freq, false) == S_OK)
			{
				D3D11_QUERY_DATA_TIMESTAMP_DISJOINT sData;
				if (SyncTimestamp(freq, &sData, true) == S_OK)
				{
					*((UINT64*)pFrequency) = sData.Frequency;
					hr = S_OK;
				}
			}
		}
		ReleaseTimestamp(freq);
	}
	return hr;
}

HRESULT CDeviceManager::CreateTimestamp(DeviceTimestampHandle& query, bool bDisjointTest)
{
	HRESULT hr = S_FALSE;
	D3D11_QUERY_DESC QDesc;
	QDesc.Query = bDisjointTest ? D3D11_QUERY_TIMESTAMP_DISJOINT : D3D11_QUERY_TIMESTAMP;
	QDesc.MiscFlags = 0;
	D3DQuery* d3d_query;
	hr = gcpRendD3D->GetDevice().CreateQuery(&QDesc, &d3d_query);
	if (CHECK_HRESULT(hr))
	{
		query = reinterpret_cast<DeviceFenceHandle>(d3d_query);
	}
	return hr;
}

HRESULT CDeviceManager::ReleaseTimestamp(DeviceTimestampHandle query)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	SAFE_RELEASE(d3d_query);
	hr = S_OK;
	return hr;
}

HRESULT CDeviceManager::IssueTimestamp(DeviceTimestampHandle query, bool begin)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	if (d3d_query)
	{
		if (begin)
			gcpRendD3D->GetDeviceContext().Begin(d3d_query);
		else
			gcpRendD3D->GetDeviceContext().End(d3d_query);
		hr = S_OK;
	}
	return hr;
}

HRESULT CDeviceManager::ResolveTimestamps(bool block, bool flush)
{
	HRESULT hr = S_FALSE;
	DeviceFenceHandle wait;
	if (CreateFence(wait) == S_OK)
	{
		if (IssueFence(wait) == S_OK)
		{
			if (SyncFence(wait, true, true) == S_OK)
			{
				hr = S_OK;
			}
		}
		ReleaseFence(wait);
	}
	return hr;
}

HRESULT CDeviceManager::SyncTimestamp(DeviceTimestampHandle query, void* pData, bool flush)
{
	HRESULT hr = S_FALSE;
	D3DQuery* d3d_query = reinterpret_cast<D3DQuery*>(query);
	if (d3d_query)
	{
		CRY_ASSERT(!flush || !gRenDev->m_pRT || gRenDev->m_pRT->IsRenderThread()); // Can only flush from render thread

		hr = gcpRendD3D->GetDeviceContext().GetData(d3d_query, pData, (flush ? 2 : 1) * sizeof(UINT64), flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
	#if !defined(_RELEASE)
		if (hr != S_OK && hr != S_FALSE)
		{
			CHECK_HRESULT(hr);
		}
	#endif
	}
	return hr;
}

HRESULT CDeviceManager::SyncTimestamps(INT offs, INT num, DeviceTimestampHandle* query, void* pData, bool flush)
{
	return S_FALSE;
}
#endif

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
	return nullptr;
}

void CDeviceManager::ReleaseNullResource(D3DResource* pNullResource)
{
	;
}

//=============================================================================

#ifdef DEVMAN_USE_STAGING_POOL
D3DResource* CDeviceManager::AllocateStagingResource(D3DResource* pForTex, bool bUpload)
{
	D3D11_TEXTURE2D_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));

	((D3DTexture*)pForTex)->GetDesc(&Desc);
	Desc.Usage = bUpload ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_STAGING;
	Desc.CPUAccessFlags = bUpload ? D3D11_CPU_ACCESS_WRITE : D3D11_CPU_ACCESS_READ;
	Desc.BindFlags = bUpload ? D3D11_BIND_SHADER_RESOURCE : 0;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	StagingPoolVec::iterator it = std::find(m_stagingPool.begin(), m_stagingPool.end(), Desc);

	if (it == m_stagingPool.end())
	{
		D3DTexture* pStagingTexture = NULL;

		gcpRendD3D->GetDevice().CreateTexture2D(&Desc, NULL, &pStagingTexture);

	#ifndef _RELEASE
		if (pStagingTexture)
		{
			D3D11_TEXTURE2D_DESC stagingDesc;
			memset(&stagingDesc, 0, sizeof(stagingDesc));

			pStagingTexture->GetDesc(&stagingDesc);
			stagingDesc.Usage = bUpload ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_STAGING;
			stagingDesc.CPUAccessFlags = bUpload ? D3D11_CPU_ACCESS_WRITE : D3D11_CPU_ACCESS_READ;
			stagingDesc.BindFlags = bUpload ? D3D11_BIND_SHADER_RESOURCE : 0;

			if (memcmp(&stagingDesc, &Desc, sizeof(Desc)) != 0)
			{
				assert(0);
			}
		}
	#endif

		return pStagingTexture;
	}
	else
	{
		D3DTexture* pStagingResource = NULL;

		pStagingResource = it->pStagingResource;
		m_stagingPool.erase(it);

		return pStagingResource;
	}
}

void CDeviceManager::ReleaseStagingResource(D3DResource* pStagingRes)
{
	D3D11_TEXTURE2D_DESC Desc;
	memset(&Desc, 0, sizeof(Desc));

	((D3DTexture*)pStagingRes)->GetDesc(&Desc);

	StagingTextureDef def;
	def.desc = Desc;
	def.pStagingResource = (D3DTexture*)pStagingRes;
	m_stagingPool.push_back(def);
}
#endif

//=============================================================================

int32 CDeviceTexture::Release()
{
	int32 nRef = Cleanup();

	assert(nRef >= 0);

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
	D3DResource* pStagingResource;
	if (!(pStagingResource = m_pStagingResource[0]))
	{
		pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, FALSE);
	}

	assert(pStagingResource);

#if defined(DEVICE_SUPPORTS_D3D11_1)
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(pStagingResource, nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion(pStagingResource, nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL);
#endif

	D3D11_MAPPED_SUBRESOURCE lrct;
	HRESULT hr = gcpRendD3D->GetDeviceContext().Map(pStagingResource, nSubRes, D3D11_MAP_READ, 0, &lrct);

	if (S_OK == hr)
	{
		const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
		gcpRendD3D->GetDeviceContext().Unmap(pStagingResource, nSubRes);
	}

	if (!(m_pStagingResource[0]))
	{
		gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
	}
}

void CDeviceTexture::DownloadToStagingResource(uint32 nSubRes)
{
	assert(m_pStagingResource[0]);

#if defined(DEVICE_SUPPORTS_D3D11_1)
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(m_pStagingResource[0], nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion(m_pStagingResource[0], nSubRes, 0, 0, 0, m_pD3DTexture, nSubRes, NULL);
#endif
}

void CDeviceTexture::UploadFromStagingResource(const uint32 nSubRes, StagingHook cbTransfer)
{
	D3DResource* pStagingResource;
	if (!(pStagingResource = m_pStagingResource[1]))
	{
		pStagingResource = gcpRendD3D->m_DevMan.AllocateStagingResource(m_pD3DTexture, TRUE);
	}

	assert(pStagingResource);

	D3D11_MAPPED_SUBRESOURCE lrct;
	HRESULT hr = gcpRendD3D->GetDeviceContext().Map(pStagingResource, nSubRes, D3D11_MAP_WRITE_DISCARD, 0, &lrct);

	if (S_OK == hr)
	{
		const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
		gcpRendD3D->GetDeviceContext().Unmap(pStagingResource, 0);
		if (update)
		{
#if defined(DEVICE_SUPPORTS_D3D11_1)
			gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(m_pD3DTexture, nSubRes, 0, 0, 0, pStagingResource, nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
			gcpRendD3D->GetDeviceContext().CopySubresourceRegion(m_pD3DTexture, nSubRes, 0, 0, 0, pStagingResource, nSubRes, NULL);
#endif
		}
	}

	if (!(m_pStagingResource[1]))
	{
		gcpRendD3D->m_DevMan.ReleaseStagingResource(pStagingResource);
	}
}

void CDeviceTexture::UploadFromStagingResource(const uint32 nSubRes)
{
	assert(m_pStagingResource[1]);

#if defined(DEVICE_SUPPORTS_D3D11_1)
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion1(m_pD3DTexture, nSubRes, 0, 0, 0, m_pStagingResource[1], nSubRes, NULL, D3D11_COPY_NO_OVERWRITE);
#else
	gcpRendD3D->GetDeviceContext().CopySubresourceRegion(m_pD3DTexture, nSubRes, 0, 0, 0, m_pStagingResource[1], nSubRes, NULL);
#endif
}

void CDeviceTexture::AccessCurrStagingResource(uint32 nSubRes, bool forUpload, StagingHook cbTransfer)
{
	D3D11_MAPPED_SUBRESOURCE lrct;
	HRESULT hr = gcpRendD3D->GetDeviceContext().Map(m_pStagingResource[forUpload], nSubRes, forUpload ? D3D11_MAP_WRITE : D3D11_MAP_READ, 0, &lrct);

	if (S_OK == hr)
	{
		const bool update = cbTransfer(lrct.pData, lrct.RowPitch, lrct.DepthPitch);
		gcpRendD3D->GetDeviceContext().Unmap(m_pStagingResource[forUpload], nSubRes);
	}
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
	if (nUsage & USAGE_STREAMING)
	{
		nMiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;
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
	Desc.MiscFlags = nMiscFlags;

#if CRY_PLATFORM_DURANGO
	if (nESRAMOffset != -1)
	{
		Desc.MiscFlags |= D3D11X_RESOURCE_MISC_ESRAM_RESIDENT;
		Desc.ESRAMOffsetBytes = (UINT)nESRAMOffset;
	}

	if (InPlaceConstructable(Desc))
	{
		bool bDeferD3DCreation = (nUsage & USAGE_STREAMING) && !(pTI && pTI->m_pData);

		hr = CreateInPlaceTexture2D(Desc, pTI ? pTI->m_pData : NULL, *ppDevTexture, bDeferD3DCreation);
	}
	else
#endif
	{
		hr = Create2DTexture(Desc, cClearValue, ppDevTexture, pTI);
	}

	if (hr == S_OK)
	{
		CDeviceTexture* pDeviceTexture = *ppDevTexture;

		if (nUsage & USAGE_STAGE_ACCESS)
		{
			if (nUsage & USAGE_CPU_READ)
			{
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
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

#if CRY_PLATFORM_DURANGO
	if (InPlaceConstructable(Desc))
	{
		bool bDeferD3DCreation = (nUsage & USAGE_STREAMING) && !(pTI && pTI->m_pData);

		hr = CreateInPlaceTexture2D(Desc, pTI ? pTI->m_pData : NULL, *ppDevTexture, bDeferD3DCreation);
	}
	else
#endif
	{
		hr = CreateCubeTexture(Desc, cClearValue, ppDevTexture, pTI);
	}

	if (hr == S_OK)
	{
		CDeviceTexture* pDeviceTexture = *ppDevTexture;

		if (nUsage & USAGE_STAGE_ACCESS)
		{
			if (nUsage & USAGE_CPU_READ)
			{
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
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
				pDeviceTexture->m_pStagingResource[0] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, FALSE);
			}

			if (nUsage & USAGE_CPU_WRITE)
			{
				pDeviceTexture->m_pStagingResource[1] = gcpRendD3D->m_DevMan.AllocateStagingResource(pDeviceTexture->m_pD3DTexture, TRUE);
			}
		}
	}

	return hr;
}

HRESULT CDeviceManager::Create2DTexture(const D3D11_TEXTURE2D_DESC& Desc, const ColorF& cClearValue, LPDEVICETEXTURE* ppDevTexture, STextureInfo* pTI)
{
	CDeviceTexture* pDeviceTexture = 0;//new CDeviceTexture();
	D3DTexture* pD3DTex = NULL;
	HRESULT hr = S_OK;

	D3D11_SUBRESOURCE_DATA* pSRD = NULL;
	D3D11_SUBRESOURCE_DATA SRD[20];
	if (pTI && pTI->m_pData)
	{
		pSRD = &SRD[0];
		for (int i = 0; i < Desc.MipLevels; i++)
		{
			SRD[i].pSysMem = pTI->m_pData[i].pSysMem;
			SRD[i].SysMemPitch = pTI->m_pData[i].SysMemPitch;
			SRD[i].SysMemSlicePitch = pTI->m_pData[i].SysMemSlicePitch;

#if CRY_PLATFORM_ORBIS
			SRD[i].SysMemTileMode = (D3D11_TEXTURE_TILE_MODE)pTI->m_pData[i].SysMemTileMode;
#endif
		}
	}

	hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);

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

#if CRY_PLATFORM_ORBIS
				SRD[nSubresInd].SysMemTileMode = (D3D11_TEXTURE_TILE_MODE)pTI->m_pData[nSubresInd].SysMemTileMode;
#endif
			}
		}
	}

	hr = gcpRendD3D->GetDevice().CreateTexture2D(&Desc, pSRD, &pD3DTex);

	if (SUCCEEDED(hr) && pDeviceTexture == 0)
	{
		pDeviceTexture = new CDeviceTexture();
	}

	if (SUCCEEDED(hr) && pD3DTex)
	{
		pDeviceTexture->m_pD3DTexture = pD3DTex;
		pDeviceTexture->m_bCube = true;
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

#if CRY_PLATFORM_ORBIS
			SRD[i].SysMemTileMode = (D3D11_TEXTURE_TILE_MODE)pTI->m_pData[i].SysMemTileMode;
#endif
		}
	}

	hr = gcpRendD3D->GetDevice().CreateTexture3D(&Desc, pSRD, &pD3DTex);

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

#if CRY_PLATFORM_DURANGO && BUFFER_USE_STAGED_UPDATES == 0
	if (nUsage & USAGE_DIRECT_ACCESS)
	{
		if (nUsage & USAGE_STAGING)
		{
			CryFatalError("staging buffers not supported if BUFFER_USE_STAGED_UPDATES not defined");
		}

		BufDesc.ByteWidth = nSize * elemSize;
		BufDesc.Usage = (D3D11_USAGE)D3D11_USAGE_DEFAULT;
		BufDesc.MiscFlags = 0;
		BufDesc.CPUAccessFlags = 0;

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

		void* BufBasePtr;

		if (!IsAligned(nSize * elemSize, 4096))
		{
			CryFatalError("Memory Allocation Size for Direct Video Memory Access must be a multiple of 4 KB but the supplied size is %u", nSize * elemSize);
		}

		D3D11_GRAPHICS_MEMORY_ACCESS_FLAG access_flag = D3D11_GRAPHICS_MEMORY_ACCESS_CPU_WRITECOMBINE_NONCOHERENT;
		if ((nUsage & (USAGE_DIRECT_ACCESS_CPU_COHERENT | USAGE_DIRECT_ACCESS_GPU_COHERENT)) ==
		    (USAGE_DIRECT_ACCESS_CPU_COHERENT | USAGE_DIRECT_ACCESS_GPU_COHERENT))
		{
			access_flag = D3D11_GRAPHICS_MEMORY_ACCESS_CPU_CACHE_COHERENT;
		}
		{
			hr = D3DAllocateGraphicsMemory(
			  nSize * elemSize,
			  0, 0
			  , access_flag
			  , &BufBasePtr);
		}
		CHECK_HRESULT(hr);

		hr = gcpRendD3D->GetPerformanceDevice().CreatePlacementBuffer(&BufDesc, BufBasePtr, ppBuff);
		CHECK_HRESULT(hr);

		UINT size = sizeof(BufBasePtr);
		hr = (*ppBuff)->SetPrivateData(BufferPointerGuid, size, &BufBasePtr);
		CHECK_HRESULT(hr);
		return hr;
	}
#endif

	BufDesc.ByteWidth = nSize * elemSize;
	int nD3DUsage = D3D11_USAGE_DEFAULT;
	nD3DUsage = (nUsage & USAGE_DYNAMIC) ? D3D11_USAGE_DYNAMIC : nD3DUsage;
	nD3DUsage = (nUsage & USAGE_IMMUTABLE) ? D3D11_USAGE_IMMUTABLE : nD3DUsage;
	nD3DUsage = (nUsage & USAGE_STAGING) ? D3D11_USAGE_STAGING : nD3DUsage;
	BufDesc.Usage = (D3D11_USAGE)nD3DUsage;

#if !defined(OPENGL)
	if (BufDesc.Usage != D3D11_USAGE_STAGING)
#endif
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
#if !defined(OPENGL)
	else
	{
		BufDesc.BindFlags = 0;
	}
#endif

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

	hr = gcpRendD3D->GetDevice().CreateBuffer(&BufDesc, NULL, ppBuff);
	CHECK_HRESULT(hr);
	return hr;
}
