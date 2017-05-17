// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../../../Common/Textures/TextureStreamPool.h" // STexPoolItem

UINT64 CTexture::StreamInsertFence()
{
	return gcpRendD3D->GetPerformanceDeviceContext().InsertFence();
}

bool CTexture::StreamPrepare_Platform()
{
	if (m_eSrcTileMode == eTM_LinearPadded)
	{
		int nSides = StreamGetNumSlices();

		// Determine which mips are in place streamable - those whose linear tiled layout sizes are <= the optimal tiled layout size

		XG_TEXTURE2D_DESC desc;
		ZeroStruct(desc);
		desc.Width = m_nWidth;
		desc.Height = m_nHeight;
		desc.MipLevels = m_nMips;
		desc.ArraySize = nSides;
		desc.Format = (XG_FORMAT)m_pPixelFormat->DeviceFormat;
		desc.SampleDesc.Count = 1;
		desc.Usage = XG_USAGE_STAGING;
		desc.BindFlags = XG_BIND_SHADER_RESOURCE;
		desc.TileMode = XG_TILE_MODE_LINEAR;

		XG_RESOURCE_LAYOUT linLayout;
		XGComputeTexture2DLayout(&desc, &linLayout);

		STexStreamingInfo* pSI = m_pFileTexMips;

		for (int nMip = 0; nMip < m_nMips; ++nMip)
		{
			STexMipHeader& mh = pSI->m_pMipHeader[nMip];
			mh.m_InPlaceStreamable = linLayout.Plane[0].MipLayout[nMip].Slice2DSizeBytes >= 16384; // Tested in RC - all surfaces larger fit in their optimal counterparts
		}
	}

	return true;
}

bool CTexture::StreamInCheckTileComplete_Durango(STexStreamInState& state)
{
	if (state.m_tileFence)
	{
		if (!gcpRendD3D->GetPerformanceDevice().IsFencePending(state.m_tileFence))
			state.m_tileFence = 0;
		else
			return false;
	}

	return true;
}

bool CTexture::StreamInCheckCopyComplete_Durango(STexStreamInState& state)
{
	if (state.m_copyMipsFence)
	{
		if (!gcpRendD3D->GetPerformanceDevice().IsFencePending(state.m_copyMipsFence))
			state.m_copyMipsFence = 0;
		else
			return false;
	}

	return true;
}

bool CTexture::StreamOutCheckComplete_Durango(STexStreamOutState& state)
{
#if defined(TEXSTRM_ASYNC_TEXCOPY)
	if (state.m_copyFence)
	{
		if (!gcpRendD3D->GetPerformanceDevice().IsFencePending(state.m_copyFence))
			state.m_copyFence = 0;
		else
			return false;
	}
#endif

	return true;
}

UINT64 CTexture::StreamCopyMipsTexToTex_MoveEngine(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDstItem, int nMipDest, int nNumMips)
{
	CDeviceTexture* pDstDevTexture = pDstItem->m_pDevTexture;
	CDeviceTexture* pSrcDevTexture = pSrcItem->m_pDevTexture;

	const int32 nSides = pDstItem->m_pOwner->GetNumSlices();

	CryAutoLock<CryCriticalSection> dmaLock(GetDeviceObjectFactory().m_dma1Lock);
	ID3D11DmaEngineContextX* pDMA = GetDeviceObjectFactory().m_pDMA1;

	// Need to pin the destination because we're going to invalidate it.
	// Need to pin both the source and destination to ensure that the D3D texture interfaces remain
	// valid whilst we issue CSR commands

	DMAPIN_DEVICE_TEXTURE(pDMA, pDstDevTexture);
	DMAPIN_DEVICE_TEXTURE(pDMA, pSrcDevTexture);

	D3DBaseTexture* pDstResource = pDstDevTexture->GetBaseTexture();
	D3DBaseTexture* pSrcResource = pSrcDevTexture->GetBaseTexture();

	for (int32 iSide = 0; iSide < nSides; ++iSide)
	{
		for (int nMip = 0; nMip < nNumMips; ++nMip)
		{
			pDMA->CopySubresourceRegion(
				pDstResource,
				D3D11CalcSubresource(nMipDest + nMip, iSide, pDstItem->m_pOwner->m_nMips),
				0, 0, 0,
				pSrcResource,
				D3D11CalcSubresource(nMipSrc + nMip, iSide, pSrcItem->m_pOwner->m_nMips),
				NULL,
				D3D11_COPY_NO_OVERWRITE);
		}
	}

	UINT64 fence = pDMA->InsertFence(D3D11_INSERT_FENCE_NO_KICKOFF);
	pDMA->Submit();
	return fence;
}

void CTexture::StreamUploadMip_Durango(const void* pSurfaceData,
	int nMip, int nBaseMipOffset,
	STexPoolItem* pNewPoolItem, STexStreamInMipState& mipState)
{
	FUNCTION_PROFILER_RENDERER;

	CDeviceTexture* pDevTex = pNewPoolItem->m_pDevTexture;
	bool bStreamInPlace = mipState.m_bStreamInPlace;

	if (pDevTex->IsInPool())
	{
		if (m_eSrcTileMode == eTM_LinearPadded)
		{
			// Is tileable by the move engine, but not just yet, as the CPU may still be using the texture
			// memory. Copy out of the way for the moment.

			StreamExpandMip(pSurfaceData, nMip, nBaseMipOffset, mipState.m_nSideDelta);
		}
		else if (m_eSrcTileMode == eTM_None)
		{
			// Linear general - can't use the move engine, so tile on the CPU :(. As the src mode is untiled,
			// all subresources for this texture will be using the CPU, so we can do this now.

			bool isBlockCompressed = IsBlockCompressed(m_eTFSrc);
			int numMips = pNewPoolItem->m_pOwner->m_nMips;
			const int numSides = StreamGetNumSlices();
			int nTexMip = nMip + nBaseMipOffset;

			XG_TEXTURE2D_DESC xgDesc;
			ZeroStruct(xgDesc);

			xgDesc.Width = pNewPoolItem->m_pOwner->m_Width;
			xgDesc.Height = pNewPoolItem->m_pOwner->m_Height;
			xgDesc.MipLevels = numMips;
			xgDesc.ArraySize = numSides;
			xgDesc.Format = (XG_FORMAT)m_pPixelFormat->DeviceFormat;
			xgDesc.SampleDesc.Count = 1;
			xgDesc.Usage = XG_USAGE_DEFAULT;
			xgDesc.BindFlags = XG_BIND_SHADER_RESOURCE;
			xgDesc.CPUAccessFlags = 0;            // Any of XG_CPU_ACCESS_FLAG.
			xgDesc.MiscFlags = ((m_eTT == eTT_Cube) ? XG_RESOURCE_MISC_TEXTURECUBE : 0);
			xgDesc.TileMode = pDevTex->GetXGTileMode();

			XGTextureAddressComputer* pComputer = NULL;
			HRESULT hr = XGCreateTexture2DComputer(&xgDesc, &pComputer);
			if (FAILED(hr))
			{
#ifndef _RELEASE
				__debugbreak();
#endif
				return;
			}

			CDeviceTexturePin pin(pDevTex);
			char* pBase = (char*)pin.GetBaseAddress();

			// Expecting that the data has been expanded, or that the src format is an inplace format (expansion is a nop)
			int nSrcSurfaceSize = TextureDataSize(m_nWidth >> nTexMip, m_nHeight >> nTexMip, 1, 1, 1, m_eTFDst);
			int nSrcSidePitch = nSrcSurfaceSize + mipState.m_nSideDelta;
			int nSrcRowPitch = TextureDataSize(m_nWidth >> nTexMip, 1, 1, 1, 1, m_eTFDst);

			for (int nSlice = 0; nSlice < numSides; ++nSlice)
			{
				int nDstSubResIdx = nMip + numMips * nSlice;
				const char* pSRSrc = pSurfaceData
					? static_cast<const char*>(pSurfaceData) + nSlice * nSrcSidePitch
					: reinterpret_cast<const char*>(m_pFileTexMips->m_pMipHeader[nTexMip].m_Mips[nSlice].DataArray);

#ifndef _RELEASE
				if (!pSRSrc)
					__debugbreak();
#endif

				pComputer->CopyIntoSubresource(pBase, 0, nDstSubResIdx, pSRSrc, nSrcRowPitch, 0);
			}

#if defined(TEXTURES_IN_CACHED_MEM)
			D3DFlushCpuCache(pBase, pDevTex->GetDeviceSize());
#endif

			mipState.m_bUploaded = true;
			mipState.m_bExpanded = false;

			pComputer->Release();
		}
		else
		{
#ifndef _RELEASE
			__debugbreak();
#endif
		}
	}
	else
	{
#ifndef _RELEASE
		__debugbreak();
#endif
	}
}

void CTexture::StreamUploadMips_Durango(int nBaseMip, int nMipCount, STexPoolItem* pNewPoolItem, STexStreamInState& streamState)
{
	FUNCTION_PROFILER_RENDERER;

	CDeviceTexture* pDevTex = pNewPoolItem->m_pDevTexture;

	if (pDevTex->IsInPool())
	{
		if (m_eSrcTileMode == eTM_LinearPadded)
		{
			// Data is in a format that the move engine can tile, wrap it in a placement texture
			// and move it.

			// Assume that the texture has been properly pinned for the stream op externally
			char* pBaseAddress = (char*)pDevTex->WeakPin();
			const int numSides = StreamGetNumSlices();

			CDeviceObjectFactory::STileRequest subRes[g_nD3D10MaxSupportedSubres];
			size_t nSubRes = 0;

			for (int nMip = nBaseMip; nMip < (nBaseMip + nMipCount); ++nMip)
			{
				STexStreamInMipState& ms = streamState.m_mips[nMip - nBaseMip];
				int nDstMip = nMip - nBaseMip;

				if (!ms.m_bUploaded)
				{
					for (int nSide = 0; nSide < numSides; ++nSide)
					{
						CDeviceObjectFactory::STileRequest& sr = subRes[nSubRes++];

						sr.nDstSubResource = D3D11CalcSubresource(nDstMip, nSide, pNewPoolItem->m_pOwner->m_nMips);
						if (ms.m_bStreamInPlace)
						{
							void* pSurface = pBaseAddress + pDevTex->GetSurfaceOffset(nDstMip, nSide);
							size_t nSurfaceSize = pDevTex->GetSurfaceSize(nDstMip);

#if defined(TEXTURES_IN_CACHED_MEM)
							D3DFlushCpuCache((void*)pSurface, nSurfaceSize);
#endif

							sr.pLinSurfaceSrc = pSurface;
							sr.bSrcInGPUMemory = true;
						}
						else
						{
							sr.pLinSurfaceSrc = m_pFileTexMips->m_pMipHeader[nMip].m_Mips[nSide].DataArray;
							sr.bSrcInGPUMemory = false;
						}

#ifndef _RELEASE
						if (!sr.pLinSurfaceSrc)
							__debugbreak();
#endif
					}

					ms.m_bUploaded = true;
					ms.m_bExpanded = false;
				}
			}

			if (nSubRes > 0)
				GetDeviceObjectFactory().BeginTileFromLinear2D(pDevTex, subRes, nSubRes, streamState.m_tileFence);
		}
	}
	else
	{
#ifndef _RELEASE
		__debugbreak();
#endif
	}
}

//=============================================================================

void CDeviceTexture::InitD3DTexture()
{
	// This really ought to be only called after streaming a texture in - all other textures should already have
	// their D3D counterpart.

	if (!m_pNativeResource)
	{
		if (!m_gpuHdl.IsValid()) __debugbreak();
			
		XG_TILE_MODE tileMode = m_pLayout->xgTileMode;
		SGPUMemHdl texHdl = m_gpuHdl;

		CDeviceTexturePin pin(this);

		HRESULT hr = gcpRendD3D->GetPerformanceDevice().CreatePlacementTexture2D(&m_pLayout->d3dDesc, tileMode, 0, pin.GetBaseAddress(), (ID3D11Texture2D**)&m_pNativeResource);
	#ifndef _RELEASE
		if (FAILED(hr))
		{
			__debugbreak();
		}
	#endif
	}
}

void CDeviceTexture::ReplaceTexture(ID3D11Texture2D* pReplacement)
{
	SAFE_RELEASE(m_pNativeResource);
	m_pNativeResource = pReplacement;
	++m_nBaseAddressInvalidated;
}

void* CDeviceTexture::WeakPin()
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		return GetDeviceObjectFactory().m_texturePool.WeakPin(m_gpuHdl);
	}
	else
		__debugbreak();
	
	return NULL;
}

void* CDeviceTexture::Pin()
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		return GetDeviceObjectFactory().m_texturePool.Pin(m_gpuHdl);
	}
	else
		__debugbreak();
	
	return NULL;
}

void CDeviceTexture::Unpin()
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		GetDeviceObjectFactory().m_texturePool.Unpin(m_gpuHdl);
	}
	else
		__debugbreak();
}

void CDeviceTexture::GpuPin()
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		GetDeviceObjectFactory().m_texturePool.GpuPin(m_gpuHdl);
	}
	else
		__debugbreak();
}

void CDeviceTexture::GpuUnpin(ID3DXboxPerformanceContext* pCtx)
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		GetDeviceObjectFactory().m_texturePool.GpuUnpin(m_gpuHdl, pCtx);
	}
	else
		__debugbreak();
}

void CDeviceTexture::GpuUnpin(ID3D11DmaEngineContextX* pCtx)
{
	// Can only pin pinnable resources
	assert(m_gpuHdl.IsValid());
	if (m_gpuHdl.IsValid())
	{
		GetDeviceObjectFactory().m_texturePool.GpuUnpin(m_gpuHdl, pCtx);
	}
	else
		__debugbreak();
}

void CDeviceTexture::GPUFlush()
{
#if DURANGO_ENABLE_ASYNC_DIPS
	gcpRendD3D->WaitForAsynchronousDevice();
#endif
	gcpRendD3D->GetPerformanceDeviceContext().FlushGpuCaches(GetBaseTexture());
}

uint32 CDeviceTexture::TextureDataSize(uint32 nWidth, uint32 nHeight, uint32 nDepth, uint32 nMips, uint32 nSlices, const ETEX_Format eTF, ETEX_TileMode eTM)
{
	FUNCTION_PROFILER_RENDERER;

	if (nMips && eTF != eTF_Unknown)
	{
		const SPixFormat* pPF;
		ETEX_Format eTFDst = gcpRendD3D->m_hwTexFormatSupport.GetClosestFormatSupported(eTF, pPF);
		if (eTFDst != eTF_Unknown && eTM != eTM_None)
		{
			XG_FORMAT xgFmt = (XG_FORMAT)pPF->DeviceFormat;
			XG_TILE_MODE xgTileMode;
			XG_USAGE xgUsage = XG_USAGE_DEFAULT;
			switch (eTM)
			{
			case eTM_LinearPadded:
				xgTileMode = XG_TILE_MODE_LINEAR;
				xgUsage = XG_USAGE_STAGING;
				break;
			case eTM_Optimal:
				xgTileMode = XGComputeOptimalTileMode(
					nDepth == 1 ? XG_RESOURCE_DIMENSION_TEXTURE2D : XG_RESOURCE_DIMENSION_TEXTURE3D,
					xgFmt,
					nWidth,
					nHeight,
					nSlices,
					1,
					XG_BIND_SHADER_RESOURCE);
				break;
			}

			if (nDepth == 1)
			{
				XG_TEXTURE2D_DESC desc;
				desc.Width = nWidth;
				desc.Height = nHeight;
				desc.MipLevels = nMips;
				desc.ArraySize = nSlices;
				desc.Format = xgFmt;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.Usage = xgUsage;
				desc.BindFlags = XG_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				desc.TileMode = xgTileMode;
				desc.Pitch = 0;

				XG_RESOURCE_LAYOUT layout;
				if (!FAILED(XGComputeTexture2DLayout(&desc, &layout)))
				{
					return (int)layout.SizeBytes;
				}
			}
			else
			{
				XG_TEXTURE3D_DESC desc;
				desc.Width = nWidth;
				desc.Height = nHeight;
				desc.Depth = nDepth;
				desc.MipLevels = nMips;
				desc.Format = xgFmt;
				desc.Usage = xgUsage;
				desc.BindFlags = XG_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = 0;
				desc.TileMode = xgTileMode;
				desc.Pitch = 0;

				XG_RESOURCE_LAYOUT layout;
				if (!FAILED(XGComputeTexture3DLayout(&desc, &layout)))
				{
					return (int)layout.SizeBytes;
				}
			}
		}

		// Fallback to the default texture size function
		return CTexture::TextureDataSize(nWidth, nHeight, nDepth, nMips, nSlices, eTF, eTM_None);
	}

	return 0;
}
