// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================
   D3DTexturesStreaming.cpp : Direct3D9 specific texture streaming technology.

   Revision history:
* Created by Honitch Andrey
   - 19:8:2008   12:14 : Refactored by Kaplanyan Anton

   =============================================================================*/

#include "StdAfx.h"
#include "DriverD3D.h"
#include <CryString/StringUtils.h>
#include <CryThreading/IJobManager_JobDelegator.h>

#include "../Common/Textures/TextureStreamPool.h"

//===============================================================================
#ifdef TEXSTRM_ASYNC_TEXCOPY
DECLARE_JOB("Texture_StreamOutCopy", TTexture_StreamOutCopy, STexStreamOutState::CopyMips);
#endif

// checks for MT-safety of called functions
#if !defined(CHK_RENDTH)
	#define CHK_RENDTH assert(gRenDev->m_pRT->IsRenderThread())
#endif
#if !defined(CHK_MAINTH)
	#define CHK_MAINTH assert(gRenDev->m_pRT->IsMainThread())
#endif
#if !defined(CHK_MAINORRENDTH)
	#define CHK_MAINORRENDTH assert(gRenDev->m_pRT->IsMainThread() || gRenDev->m_pRT->IsRenderThread() || gRenDev->m_pRT->IsLevelLoadingThread())
#endif

void CTexture::InitStreamingDev()
{
#if defined(TEXSTRM_DEFERRED_UPLOAD)
	if (CRenderer::CV_r_texturesstreamingDeferred)
	{
		if (!s_pStreamDeferredCtx)
		{
			gcpRendD3D->GetDevice().CreateDeferredContext(0, &s_pStreamDeferredCtx);
		}
	}
#endif
}

bool CTexture::IsStillUsedByGPU()
{
	CDeviceTexture* pDeviceTexture = m_pDevTexture;
	if (pDeviceTexture)
	{
		CHK_RENDTH;
		D3DBaseTexture* pD3DTex = pDeviceTexture->GetBaseTexture();
	}
	return false;
}

#if CRY_PLATFORM_DURANGO

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
		desc.Format = (XG_FORMAT)m_pPixelFormat->DeviceFormat,
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

#else

bool CTexture::StreamPrepare_Platform()
{
	return true;
}

#endif

#if CRY_PLATFORM_DURANGO
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

			CDeviceManager::STileRequest subRes[g_nD3D10MaxSupportedSubres];
			size_t nSubRes = 0;

			for (int nMip = nBaseMip; nMip < (nBaseMip + nMipCount); ++nMip)
			{
				STexStreamInMipState& ms = streamState.m_mips[nMip - nBaseMip];
				int nDstMip = nMip - nBaseMip;

				if (!ms.m_bUploaded)
				{
					for (int nSide = 0; nSide < numSides; ++nSide)
					{
						CDeviceManager::STileRequest& sr = subRes[nSubRes++];

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
				gcpRendD3D->m_DevMan.BeginTileFromLinear2D(pDevTex, subRes, nSubRes, streamState.m_tileFence);
		}
	}
	else
	{
	#ifndef _RELEASE
		__debugbreak();
	#endif
	}
}
#endif

void CTexture::StreamExpandMip(const void* vpRawData, int nMip, int nBaseMipOffset, int nSideDelta)
{
	FUNCTION_PROFILER_RENDERER;

	const uint32 nCurMipWidth = (m_nWidth >> (nMip + nBaseMipOffset));
	const uint32 nCurMipHeight = (m_nHeight >> (nMip + nBaseMipOffset));

	const STexMipHeader& mh = m_pFileTexMips->m_pMipHeader[nBaseMipOffset + nMip];

	const byte* pRawData = (const byte*)vpRawData;

	const int nSides = StreamGetNumSlices();
	const bool bIsDXT = CTexture::IsBlockCompressed(m_eTFSrc);
	const int nMipAlign = bIsDXT ? 4 : 1;

	const int nSrcSurfaceSize = CTexture::TextureDataSize(nCurMipWidth, nCurMipHeight, 1, 1, 1, m_eTFSrc, m_eSrcTileMode);
	const int nSrcSidePitch = nSrcSurfaceSize + nSideDelta;

	SRenderThread* pRT = gRenDev->m_pRT;

	for (int iSide = 0; iSide < nSides; ++iSide)
	{
		SMipData* mp = &mh.m_Mips[iSide];
		if (!mp->DataArray)
			mp->Init(mh.m_SideSize, Align(max(1, m_nWidth >> nMip), nMipAlign), Align(max(1, m_nHeight >> nMip), nMipAlign));

		const byte* pRawSideData = pRawData + nSrcSidePitch * iSide;
		CTexture::ExpandMipFromFile(&mp->DataArray[0], mh.m_SideSize, pRawSideData, nSrcSurfaceSize, m_eTFSrc);
	}
}

#ifdef TEXSTRM_ASYNC_TEXCOPY
void STexStreamOutState::CopyMips()
{
	CTexture* tp = m_pTexture;

	if (m_nStartMip < MAX_MIP_LEVELS)
	{
		const int nOldMipOffset = m_nStartMip - tp->m_nMinMipVidUploaded;
		const int nNumMips = tp->GetNumMipsNonVirtual() - m_nStartMip;
	#if CRY_PLATFORM_DURANGO
		m_pNewPoolItem->m_pDevTexture->InitD3DTexture();
		m_copyFence = CTexture::StreamCopyMipsTexToTex_MoveEngine(tp->m_pFileTexMips->m_pPoolItem, 0 + nOldMipOffset, m_pNewPoolItem, 0, nNumMips);
	#else
		CTexture::StreamCopyMipsTexToTex(tp->m_pFileTexMips->m_pPoolItem, 0 + nOldMipOffset, m_pNewPoolItem, 0, nNumMips);
	#endif
	}
	else
	{
		// Stream unload case - pull persistent mips into local memory
		tp->StreamCopyMipsTexToMem(tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent, tp->m_nMips - 1, false, NULL);
	}

	m_bDone = true;
}
#endif

int CTexture::StreamTrim(int nToMip)
{
	FUNCTION_PROFILER_RENDERER;
	CHK_RENDTH;

	if (IsUnloaded() || !IsStreamed() || IsStreaming())
		return 0;

	// clamp mip level
	nToMip = max(0, min(nToMip, m_nMips - m_CacheFileHeader.m_nMipsPersistent));

	if (m_nMinMipVidUploaded >= nToMip)
		return 0;

	int nFreeSize = StreamComputeDevDataSize(m_nMinMipVidUploaded) - StreamComputeDevDataSize(nToMip);

#ifndef _RELEASE
	if (CRenderer::CV_r_TexturesStreamingDebug == 2)
		iLog->Log("Shrinking texture: %s - From mip: %i, To mip: %i", m_SrcName.c_str(), m_nMinMipVidUploaded, GetRequiredMipNonVirtual());
#endif

	STexPoolItem* pNewPoolItem = StreamGetPoolItem(nToMip, m_nMips - nToMip, false, false, true, true);
	assert(pNewPoolItem != m_pFileTexMips->m_pPoolItem);
	if (pNewPoolItem)
	{
		const int nOldMipOffset = nToMip - m_nMinMipVidUploaded;
		const int nNumMips = GetNumMipsNonVirtual() - nToMip;

#ifdef TEXSTRM_ASYNC_TEXCOPY

		bool bCopying = false;

		if (CanAsyncCopy() && (TryAddRef() > 0))
		{
			STexStreamOutState* pStreamState = StreamState_AllocateOut();
			if (pStreamState)
			{
				pStreamState->m_nStartMip = nToMip;
				pStreamState->m_pNewPoolItem = pNewPoolItem;
				pStreamState->m_pTexture = this;

				SetStreamingInProgress(StreamOutMask | (uint8)s_StreamOutTasks.GetIdxFromPtr(pStreamState));

				TTexture_StreamOutCopy job;
				job.SetClassInstance(pStreamState);
				job.RegisterJobState(&pStreamState->m_jobState);
				job.Run();

				bCopying = true;

	#ifdef DO_RENDERLOG
				if (gRenDev->m_LogFileStr)
					gRenDev->LogStrv(0, "Async Start SetLod '%s', Lods: [%d-%d], Time: %.3f\n", m_SrcName.c_str(), nToMip, m_nMips - 1, iTimer->GetAsyncCurTime());
	#endif
			}
			else
			{
				Release();
			}
		}

		if (!bCopying)
#endif
		{
#if CRY_PLATFORM_DURANGO
			pNewPoolItem->m_pDevTexture->InitD3DTexture();
#endif
			// it is a sync operation anyway, so we do it in the render thread
			CTexture::StreamCopyMipsTexToTex(m_pFileTexMips->m_pPoolItem, 0 + nOldMipOffset, pNewPoolItem, 0, nNumMips);
			StreamAssignPoolItem(pNewPoolItem, nToMip);
		}
	}
	else
	{
		s_pTextureStreamer->FlagOutOfMemory();
	}

	return nFreeSize;
}

int CTexture::StreamUnload()
{
	CHK_RENDTH;

	if (IsUnloaded() || !IsStreamed() || !CRenderer::CV_r_texturesstreaming)
		return 0;

	AbortStreamingTasks(this);
	assert(!IsStreaming());

	int nDevSize = m_nActualSize;

#ifdef TEXSTRM_ASYNC_TEXCOPY
	bool bCopying = false;

	if (CanAsyncCopy() && (TryAddRef() > 0))
	{
		STexStreamOutState* pStreamState = StreamState_AllocateOut();
		if (pStreamState)
		{
			pStreamState->m_pTexture = this;
			pStreamState->m_nStartMip = MAX_MIP_LEVELS;

			SetStreamingInProgress(StreamOutMask | (uint8)s_StreamOutTasks.GetIdxFromPtr(pStreamState));

			TTexture_StreamOutCopy job;
			job.SetClassInstance(pStreamState);
			job.RegisterJobState(&pStreamState->m_jobState);
			job.Run();

			bCopying = true;
		}
		else
		{
			Release();
		}
	}

	if (!bCopying)
#endif
	{
		// copy old mips to system memory
		StreamCopyMipsTexToMem(m_nMips - m_CacheFileHeader.m_nMipsPersistent, m_nMips - 1, false, NULL);
		ReleaseDeviceTexture(true);
		SetWasUnload(true);
	}

#ifndef _RELEASE
	if (CRenderer::CV_r_TexturesStreamingDebug == 2)
		iLog->Log("Unloading unused texture: %s", m_SrcName.c_str());
#endif

	return nDevSize;
}

void CTexture::StreamActivateLod(int nMinMip)
{
	FUNCTION_PROFILER_RENDERER;

	STexPoolItem* pItem = m_pFileTexMips->m_pPoolItem;
	STexPool* pPool = pItem->m_pOwner;
	int nMipOffset = m_nMips - pPool->m_nMips;
	int nDevMip = min((int)pPool->m_nMips - 1, max(0, nMinMip - nMipOffset));

	if (pItem->m_nActiveLod != nDevMip)
	{
		pItem->m_nActiveLod = nDevMip;

		//D3DDevice* dv = gcpRendD3D->GetD3DDevice();
		//gcpRendD3D->GetDeviceContext().SetResourceMinLOD(m_pDevTexture->GetBaseTexture(), (FLOAT)nDevMip);
	}

	m_nMinMipVidActive = nMinMip;
}

void CTexture::StreamCopyMipsTexToMem(int nStartMip, int nEndMip, bool bToDevice, STexPoolItem* pNewPoolItem)
{
	PROFILE_FRAME(Texture_StreamUpload);

	CD3D9Renderer* r = gcpRendD3D;
	CDeviceManager* pDevMan = &r->m_DevMan;
	HRESULT h = S_OK;
	nEndMip = min(nEndMip + 1, (int)m_nMips) - 1;//+1 -1 needed as the compare is <=
	STexMipHeader* mh = m_pFileTexMips->m_pMipHeader;

	const bool bIsDXT = CTexture::IsBlockCompressed(m_eTFSrc);
	const int nMipAlign = bIsDXT ? 4 : 1;

	const int nOldMinMipVidUploaded = m_nMinMipVidUploaded;

	if (bToDevice && !pNewPoolItem)
		SetMinLoadedMip(nStartMip);

	D3DFormat fmt = DeviceFormatFromTexFormat(GetDstFormat());
	if (m_bIsSRGB)
		fmt = ConvertToSRGBFmt(fmt);

	CDeviceTexture* pDevTexture = m_pDevTexture;
	uint32 nTexMips = m_nMips;
	if (m_pFileTexMips->m_pPoolItem)
	{
		assert(m_pFileTexMips->m_pPoolItem->m_pDevTexture);
		assert(pDevTexture == m_pFileTexMips->m_pPoolItem->m_pDevTexture);
		nTexMips = m_pFileTexMips->m_pPoolItem->m_pOwner->m_nMips;
	}
	if (bToDevice && pNewPoolItem)
	{
		if (m_pDevTexture)
			m_pDevTexture->SetOwner(NULL);

		assert(pNewPoolItem->m_pDevTexture);
		pDevTexture = pNewPoolItem->m_pDevTexture;
		nTexMips = pNewPoolItem->m_pOwner->m_nMips;

		if (m_pDevTexture)
			m_pDevTexture->SetOwner(this);
	}

	if (!pDevTexture)
	{
		if (m_eTT != eTT_Cube)
			h = pDevMan->Create2DTexture(m_nWidth, m_nHeight, m_nMips, m_nArraySize, STREAMED_TEXTURE_USAGE, m_cClearColor, fmt, (D3DPOOL)0, &pDevTexture);
		else
			h = pDevMan->CreateCubeTexture(m_nWidth, m_nMips, 1, STREAMED_TEXTURE_USAGE, m_cClearColor, fmt, (D3DPOOL)0, &pDevTexture);

		assert(h == S_OK);

		// If a pool item was provided, it would have a valid dev texture, so we shouldn't have ended up here..
		assert(!bToDevice || !pNewPoolItem);
		SetDevTexture(pDevTexture);
		m_pDevTexture->SetOwner(this);
	}

	if (CRenderer::CV_r_texturesstreamingnoupload && bToDevice)
		return;

	const int nMipOffset = m_nMips - nTexMips;
	const int32 nSides = StreamGetNumSlices();

	D3DBaseTexture* pID3DTexture = pDevTexture->GetBaseTexture();

	int SizeToLoad = 0;
	for (int32 iSide = 0; iSide < nSides; ++iSide)
	{
		for (int nLod = nStartMip; nLod <= nEndMip; nLod++)
		{
			SMipData* mp = &mh[nLod].m_Mips[iSide];
			int nMipW = m_nWidth >> nLod;
			int nMipH = m_nHeight >> nLod;

			if (bToDevice && !mp->DataArray && s_bStreamDontKeepSystem)  // we have this mip already loaded
				continue;

			const int nDevTexMip = nLod - nMipOffset;

			if (bToDevice)
			{
				if (mp->DataArray)
				{
					CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[nLod].m_SideSize);
					const int nRowPitch = CTexture::TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFDst);
					const int nSlicePitch = CTexture::TextureDataSize(nMipW, nMipH, 1, 1, 1, m_eTFDst);
					STALL_PROFILER("update texture");

					gcpRendD3D->GetDeviceContext().UpdateSubresource(pID3DTexture, D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), NULL, &mp->DataArray[0], nRowPitch, nSlicePitch);
				}
				else
					assert(0);
			}
			else
			{
				const int nMipSize = mh[nLod].m_SideSize;
				mp->Init(nMipSize, Align(max(1, nMipW), nMipAlign), Align(max(1, nMipH), nMipAlign));
				const int nRowPitch = CTexture::TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFDst);
				const int nRows = nMipSize / nRowPitch;
				assert(nMipSize % nRowPitch == 0);

				STALL_PROFILER("update texture");

				pDevTexture->DownloadToStagingResource(D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), [&](void* pData, uint32 rowPitch, uint32 slicePitch)
				{
					for (int iRow = 0; iRow < nRows; ++iRow)
					{
					  memcpy(&mp->DataArray[iRow * nRowPitch], (byte*)pData + rowPitch * iRow, nRowPitch);
					}

					return true;
				});

				// mark as native
				mp->m_bNative = true;
			}
			SizeToLoad += m_pFileTexMips->m_pMipHeader[nLod].m_SideSize;

			if (s_bStreamDontKeepSystem && bToDevice)
				mp->Free();
		}
	}
#ifdef DO_RENDERLOG
	if (gRenDev->m_LogFileStr)
		gRenDev->LogStrv("Uploading mips '%s'. (%d[%d]), Size: %d, Time: %.3f\n", m_SrcName.c_str(), nStartMip, m_nMips, SizeToLoad, iTimer->GetAsyncCurTime());
#endif
}

#if defined(TEXSTRM_DEFERRED_UPLOAD)

ID3D11CommandList* CTexture::StreamCreateDeferred(int nStartMip, int nEndMip, STexPoolItem* pNewPoolItem, STexPoolItem* pSrcPoolItem)
{
	PROFILE_FRAME(Texture_StreamCreateDeferred);

	ID3D11CommandList* pCmdList = NULL;

	if (CTexture::s_pStreamDeferredCtx)
	{
		CD3D9Renderer* r = gcpRendD3D;
		CDeviceManager* pDevMan = &r->m_DevMan;
		HRESULT h = S_OK;
		nEndMip = min(nEndMip + 1, (int)m_nMips) - 1;//+1 -1 needed as the compare is <=
		STexMipHeader* mh = m_pFileTexMips->m_pMipHeader;

		const int nOldMinMipVidUploaded = m_nMinMipVidUploaded;

		D3DFormat fmt = DeviceFormatFromTexFormat(GetDstFormat());
		if (m_bIsSRGB)
			fmt = ConvertToSRGBFmt(fmt);

		CDeviceTexture* pDevTexture = pNewPoolItem->m_pDevTexture;
		uint32 nTexMips = pNewPoolItem->m_pOwner->m_nMips;

		const int nMipOffset = m_nMips - nTexMips;
		const int32 nSides = StreamGetNumSlices();

		D3DBaseTexture* pID3DTexture = pDevTexture->GetBaseTexture();

		int SizeToLoad = 0;
		for (int32 iSide = 0; iSide < nSides; ++iSide)
		{
			const int32 iSideLockIndex = m_eTT != eTT_Cube ? -1 : iSide;
			for (int nLod = nStartMip; nLod <= nEndMip; nLod++)
			{
				SMipData* mp = &mh[nLod].m_Mips[iSide];

				if (mp->DataArray)
				{
					const int nDevTexMip = nLod - nMipOffset;

					CryInterlockedAdd(&CTexture::s_nTexturesDataBytesUploaded, mh[nLod].m_SideSize);
					const int nUSize = m_nWidth >> nLod;
					const int nVSize = m_nHeight >> nLod;
					const int nRowPitch = CTexture::TextureDataSize(nUSize, 1, 1, 1, 1, m_eTFDst);
					const int nSlicePitch = CTexture::TextureDataSize(nUSize, nVSize, 1, 1, 1, m_eTFDst);
					STALL_PROFILER("update texture");
					{
						s_pStreamDeferredCtx->UpdateSubresource(pID3DTexture, D3D11CalcSubresource(nDevTexMip, iSide, nTexMips), NULL, &mp->DataArray[0], nRowPitch, nSlicePitch);
					}

					SizeToLoad += m_pFileTexMips->m_pMipHeader[nLod].m_SideSize;

					if (s_bStreamDontKeepSystem)
						mp->Free();
				}
			}
		}

		int nMipsSrc = pSrcPoolItem->m_pOwner->m_nMips;
		int nMipsDst = pNewPoolItem->m_pOwner->m_nMips;
		int nMipSrcOffset = m_nMips - nMipsSrc;
		int nMipDstOffset = m_nMips - nMipsDst;

		for (int32 iSide = 0; iSide < nSides; ++iSide)
		{
			for (int i = nEndMip + 1; i < m_nMips; ++i)
			{
				s_pStreamDeferredCtx->CopySubresourceRegion(
				  pID3DTexture,
				  D3D11CalcSubresource(i - nMipDstOffset, iSide, nMipsDst),
				  0, 0, 0, pSrcPoolItem->m_pDevTexture->GetBaseTexture(),
				  D3D11CalcSubresource(i - nMipSrcOffset, iSide, nMipsSrc),
				  NULL);
			}
		}

		s_pStreamDeferredCtx->FinishCommandList(FALSE, &pCmdList);
	}

	return pCmdList;
}

void CTexture::StreamApplyDeferred(ID3D11CommandList* pCmdList)
{
	FUNCTION_PROFILER_RENDERER;
	gcpRendD3D->GetDeviceContext().ExecuteCommandList(pCmdList, TRUE);
}

#endif

#if CRY_PLATFORM_DURANGO
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
#endif

// Just remove item from the texture object and keep Item in Pool list for future use
// This function doesn't release API texture
void CTexture::StreamRemoveFromPool()
{
	CHK_MAINORRENDTH;

	if (!m_pFileTexMips || !m_pFileTexMips->m_pPoolItem)
		return;

	AUTO_LOCK(STexPoolItem::s_sSyncLock);

	m_pDeviceRTV = nullptr;
	m_pDeviceRTVMS = nullptr;
	m_pDeviceShaderResource = nullptr;
	m_pDeviceShaderResourceSRGB = nullptr;

	assert(m_pRenderTargetData == nullptr);
	SAFE_DELETE(m_pResourceViewData);

	ptrdiff_t nSize = (ptrdiff_t)m_nActualSize;
	ptrdiff_t nPersSize = (ptrdiff_t)m_nPersistentSize;

	s_pPoolMgr->ReleaseItem(m_pFileTexMips->m_pPoolItem);

	m_pFileTexMips->m_pPoolItem = NULL;
	m_nActualSize = 0;
	m_nPersistentSize = 0;
	if (m_pDevTexture)
		m_pDevTexture->SetOwner(NULL);
	m_pDevTexture = NULL;

	SetMinLoadedMip(MAX_MIP_LEVELS);
	m_nMinMipVidActive = MAX_MIP_LEVELS;

	CryInterlockedAdd(&s_nStatsStreamPoolBoundMem, -nSize);
	CryInterlockedAdd(&s_nStatsStreamPoolBoundPersMem, -nPersSize);
}

void CTexture::StreamAssignPoolItem(STexPoolItem* pItem, int nMinMip)
{
	FUNCTION_PROFILER_RENDERER;

	assert(!pItem->IsFree());
	STexPool* pItemOwner = pItem->m_pOwner;

	if (m_pFileTexMips->m_pPoolItem == pItem)
	{
		assert(m_nActualSize == m_pFileTexMips->m_pPoolItem->m_pOwner->m_Size);
		assert(m_pFileTexMips->m_pPoolItem->m_pTex == this);
		assert(m_pDevTexture == m_pFileTexMips->m_pPoolItem->m_pDevTexture);
		return;
	}

	if (m_pFileTexMips->m_pPoolItem == NULL)
	{
		if (m_pDevTexture)
			__debugbreak();
	}

	int nPersMip = m_nMips - m_CacheFileHeader.m_nMipsPersistent;
	size_t nPersSize = StreamComputeDevDataSize(nPersMip);

	// Assign a new pool item
	{
		AUTO_LOCK(STexPoolItem::s_sSyncLock);
		StreamRemoveFromPool();

		m_pFileTexMips->m_pPoolItem = pItem;
		m_nActualSize = pItemOwner->m_Size;
		m_nPersistentSize = nPersSize;
		pItem->m_pTex = this;
	}

#if CRY_PLATFORM_DURANGO
	pItem->m_pDevTexture->InitD3DTexture();
#endif

	assert(m_pDeviceRTV == nullptr);
	assert(m_pDeviceRTVMS == nullptr);
	assert(m_pDeviceShaderResource == nullptr);
	assert(m_pDeviceShaderResourceSRGB == nullptr);

	assert(m_pRenderTargetData == nullptr);
	SAFE_DELETE(m_pResourceViewData);
	SAFE_RELEASE(m_pDevTexture);

	m_pResourceViewData = new ResourceViewData();
	m_pDevTexture = pItem->m_pDevTexture;
	if (m_pDevTexture)
		m_pDevTexture->SetOwner(this);

	SetShaderResourceView((D3DShaderResource*)GetResourceView(SResourceView::ShaderResourceView(GetDstFormat(), 0, m_nArraySize, 0, -1, m_bIsSRGB, false)), false);

#if defined(CRY_PLATFORM_DURANGO)
	m_nDeviceAddressInvalidated = m_pDevTexture->GetBaseAddressInvalidated();
#endif

	SetMinLoadedMip(m_nMips - pItemOwner->m_nMips);
	StreamActivateLod(nMinMip);

	CryInterlockedAdd(&s_nStatsStreamPoolBoundMem, pItem->m_nDeviceTexSize);
	CryInterlockedAdd(&s_nStatsStreamPoolBoundPersMem, nPersSize);
}

STexPool* CTexture::StreamGetPool(int nStartMip, int nMips)
{
	const bool bIsDXT = CTexture::IsBlockCompressed(m_eTFSrc);
	const int nMipAlign = bIsDXT ? 4 : 1;
	int uSize = Align(max(1, m_nWidth >> nStartMip), nMipAlign);
	int vSize = Align(max(1, m_nHeight >> nStartMip), nMipAlign);

	return s_pPoolMgr->GetPool(uSize, vSize, nMips, m_nArraySize, m_eTFDst, m_bIsSRGB, m_eTT);
}

STexPoolItem* CTexture::StreamGetPoolItem(int nStartMip, int nMips, bool bShouldBeCreated, bool bCreateFromMipData, bool bCanCreate, bool bForStreamOut)
{
	FUNCTION_PROFILER_RENDERER;

	if (!m_pFileTexMips)
		return NULL;

	assert(nStartMip < m_nMips);
	assert(!IsStreaming());

	SCOPED_RENDERER_ALLOCATION_NAME_HINT(GetSourceName());

	const bool bIsBC = CTexture::IsBlockCompressed(m_eTFSrc);
	const int nMipAlign = bIsBC ? 4 : 1;
	int uSize = Align(max(1, m_nWidth >> nStartMip), nMipAlign);
	int vSize = Align(max(1, m_nHeight >> nStartMip), nMipAlign);
	int nArraySize = m_nArraySize;
	ETEX_Type texType = m_eTT;

	if (m_pFileTexMips->m_pPoolItem && m_pFileTexMips->m_pPoolItem->m_pOwner)
	{
		STexPoolItem* pPoolItem = m_pFileTexMips->m_pPoolItem;
		if (pPoolItem->m_pOwner->m_nMips == nMips &&
		    pPoolItem->m_pOwner->m_Width == uSize &&
		    pPoolItem->m_pOwner->m_Height == vSize &&
		    pPoolItem->m_pOwner->m_nArraySize == nArraySize)
		{
			return NULL;
		}
	}

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Texture, 0, "Creating Texture");
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Texture, 0, "%s %ix%ix%i %08x", m_SrcName.c_str(), m_nWidth, m_nHeight, m_nMips, m_nFlags);

	STextureInfo ti;
	std::vector<STextureInfoData> srti;
	STextureInfo* pTI = NULL;

	if (bCreateFromMipData)
	{
		uint32 nSlices = StreamGetNumSlices();

		ti.m_nMSAAQuality = 0;
		ti.m_nMSAASamples = 1;
		srti.resize(nSlices * nMips);
		ti.m_pData = &srti[0];

		for (int nSide = 0; nSide < nSlices; ++nSide)
		{
			for (int nMip = nStartMip, nEndMip = nStartMip + nMips; nMip != nEndMip; ++nMip)
			{
				int nSRIdx = nSide * nMips + (nMip - nStartMip);
				STexMipHeader& mmh = m_pFileTexMips->m_pMipHeader[nMip];
				SMipData& md = mmh.m_Mips[nSide];

				if (md.m_bNative)
				{
					ti.m_pData[nSRIdx].pSysMem = md.DataArray;
					ti.m_pData[nSRIdx].SysMemPitch = 0;
					ti.m_pData[nSRIdx].SysMemSlicePitch = 0;
					ti.m_pData[nSRIdx].SysMemTileMode = m_eSrcTileMode;
				}
				else
				{
					int nMipW = m_nWidth >> nMip;
					int nMipH = m_nHeight >> nMip;
					int nRowPitch = TextureDataSize(nMipW, 1, 1, 1, 1, m_eTFSrc);
					int nSlicePitch = TextureDataSize(nMipW, nMipH, 1, 1, 1, m_eTFSrc, m_eSrcTileMode);

					ti.m_pData[nSRIdx].pSysMem = md.DataArray;
					ti.m_pData[nSRIdx].SysMemPitch = nRowPitch;
					ti.m_pData[nSRIdx].SysMemSlicePitch = nSlicePitch;
					ti.m_pData[nSRIdx].SysMemTileMode = eTM_None;
				}
			}
		}

		pTI = &ti;
	}

	// For end of C3, preserve existing (idle wait) console behaviour
	bool bGPIMustWaitForIdle = !bForStreamOut;

	STexPoolItem* pItem = s_pPoolMgr->GetPoolItem(uSize, vSize, nMips, nArraySize, GetDstFormat(), m_bIsSRGB, texType, bShouldBeCreated, m_SrcName.c_str(), pTI, bCanCreate, bGPIMustWaitForIdle);
	if (pItem)
		return pItem;

	s_pTextureStreamer->FlagOutOfMemory();
	return NULL;
}

void CTexture::StreamCopyMipsTexToTex(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips)
{
	CHK_RENDTH;

	CDeviceTexture* pSrcDevTexture = pSrcItem->m_pDevTexture;
	CDeviceTexture* pDstDevTexture = pDestItem->m_pDevTexture;

	uint32 nSides = pDestItem->m_pOwner->GetNumSlices();

	if (0)
	{
	}
#if defined(CRY_PLATFORM_DURANGO)
	else if (!gcpRendD3D->m_pRT->IsRenderThread())
	{
		// We can use the move engine!

		CryAutoLock<CryCriticalSection> dmaLock(gcpRendD3D->m_dma1Lock);

		ID3D11DmaEngineContextX* pDMA = gcpRendD3D->m_pDMA1;

		// Need to pin the destination because we're going to invalidate it.
		// Need to pin both the source and destination to ensure that the D3D texture interfaces remain
		// valid whilst we issue CSR commands

		DMAPIN_DEVICE_TEXTURE(pDMA, pSrcDevTexture);
		DMAPIN_DEVICE_TEXTURE(pDMA, pDstDevTexture);

		D3DBaseTexture* pSrcResource = pSrcDevTexture->GetBaseTexture();
		D3DBaseTexture* pDstResource = pDstDevTexture->GetBaseTexture();

		for(int32 iSide = 0;iSide < nSides;++iSide)
		{
			for(int nMip=0;nMip<nNumMips;++nMip)
			{
				PREFAST_SUPPRESS_WARNING(6387)
				pDMA->CopySubresourceRegion(
					pDstResource,
					D3D11CalcSubresource(nMipDest + nMip, iSide, pDestItem->m_pOwner->m_nMips),
					0, 0, 0,
					pSrcResource,
					D3D11CalcSubresource(nMipSrc + nMip, iSide, pSrcItem->m_pOwner->m_nMips),
					NULL,
					D3D11_COPY_NO_OVERWRITE);
			}
		}

#ifdef DURANGO_MONOD3D_DRIVER
		UINT64 fence = pDMA->InsertFence(D3D11_INSERT_FENCE_NO_KICKOFF);
#else
		UINT64 fence = pDMA->InsertFence();
#endif
		pDMA->Submit();
		while (gcpRendD3D->GetPerformanceDevice().IsFencePending(fence))
			CrySleep(1);
	}
#endif
	else
	{
		GPUPIN_DEVICE_TEXTURE(gcpRendD3D->GetPerformanceDeviceContext(), pDstDevTexture);

		for (uint32 iSide = 0; iSide < nSides; ++iSide)
			CopySliceChain(pDstDevTexture, pDestItem->m_pOwner->m_nMips, iSide, nMipDest, pSrcDevTexture, iSide, nMipSrc, pSrcItem->m_pOwner->m_nMips, nNumMips);
	}
}

#if CRY_PLATFORM_DURANGO

UINT64 CTexture::StreamInsertFence()
{
	return gcpRendD3D->GetPerformanceDeviceContext().InsertFence();
}

UINT64 CTexture::StreamCopyMipsTexToTex_MoveEngine(STexPoolItem* pSrcItem, int nMipSrc, STexPoolItem* pDestItem, int nMipDest, int nNumMips)
{
	CDeviceTexture* pDstDevTexture = pDestItem->m_pDevTexture;
	CDeviceTexture* pSrcDevTexture = pSrcItem->m_pDevTexture;

	const int32 nSides = pDestItem->m_pOwner->GetNumSlices();

	CryAutoLock<CryCriticalSection> dmaLock(gcpRendD3D->m_dma1Lock);

	ID3D11DmaEngineContextX* pDMA = gcpRendD3D->m_pDMA1;

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
			  D3D11CalcSubresource(nMipDest + nMip, iSide, pDestItem->m_pOwner->m_nMips),
			  0, 0, 0,
			  pSrcResource,
			  D3D11CalcSubresource(nMipSrc + nMip, iSide, pSrcItem->m_pOwner->m_nMips),
			  NULL,
			  D3D11_COPY_NO_OVERWRITE);
		}
	}

	#ifdef DURANGO_MONOD3D_DRIVER
	UINT64 fence = pDMA->InsertFence(D3D11_INSERT_FENCE_NO_KICKOFF);
	#else
	UINT64 fence = pDMA->InsertFence();
	#endif
	pDMA->Submit();
	return fence;
}

#endif

// Debug routines /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _RELEASE
struct StreamDebugSortFunc
{
	bool operator()(const CTexture* p1, const CTexture* p2) const
	{
		if (!p1)
			return true;
		if (!p2)
			return false;
		if (p1->GetActualSize() != p2->GetActualSize())
			return p1->GetActualSize() < p2->GetActualSize();
		return p1 < p2;
	}
};

struct StreamDebugSortWantedFunc
{
	bool operator()(const CTexture* p1, const CTexture* p2) const
	{
		int nP1ReqMip = p1->GetRequiredMipNonVirtual();
		int nP1Size = p1->StreamComputeDevDataSize(nP1ReqMip);

		int nP2ReqMip = p2->GetRequiredMipNonVirtual();
		int nP2Size = p2->StreamComputeDevDataSize(nP2ReqMip);

		if (nP1Size != nP2Size)
			return nP1Size < nP2Size;

		return p1 < p2;
	}
};

void CTexture::OutputDebugInfo()
{
	CD3D9Renderer* r = gcpRendD3D;

	int nX = 40;
	int nY = 30;

	if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
	{
		Vec3i vCamPos = iSystem->GetViewCamera().GetPosition();
		CryLogAlways("===================== Dumping textures streaming debug info for camera position (%d, %d, %d) =====================", vCamPos.x, vCamPos.y, vCamPos.z);
	}

	char szText[512];
	cry_sprintf(szText, "Size | WantedSize |  MF   | P |  Mips  |  N  | F | Z | S | Name");
	IRenderAuxText::WriteXY(nX, nY, 1.f, 1.f, 1, 1, 0, 1, "%s", szText);
	if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
	{
		CryLogAlways("%s", szText);
	}

	std::vector<CTexture*> texSorted;
	s_pTextureStreamer->StatsFetchTextures(texSorted);

	const char* const sTexFilter = CRenderer::CV_r_TexturesStreamingDebugfilter->GetString();
	const bool bNameFilter = sTexFilter != 0 && strlen(sTexFilter) > 1;

	if (!texSorted.empty())
	{
		switch (CRenderer::CV_r_TexturesStreamingDebug)
		{
		case 4:
			std::stable_sort(&texSorted[0], &texSorted[0] + texSorted.size(), StreamDebugSortFunc());
			break;

		case 5:
			std::reverse(texSorted.begin(), texSorted.end());
			break;

		case 6:
			std::stable_sort(&texSorted[0], &texSorted[0] + texSorted.size(), StreamDebugSortWantedFunc());
			break;
		}
	}

	SThreadInfo& ti = gRenDev->m_RP.m_TI[gRenDev->m_pRT->GetThreadList()];

	for (int i = (int)texSorted.size() - 1, nTexNum = 0; i >= 0; --i)
	{
		CTexture* tp = texSorted[i];
		if (tp == NULL)
			continue;
		// name filter
		if (bNameFilter && !strstr(tp->m_SrcName.c_str(), sTexFilter))
			continue;
		if (tp->m_nActualSize / 1024 < CRenderer::CV_r_TexturesStreamingDebugMinSize)
			continue;

		ColorF color = (tp->m_nActualSize / 1024 >= CRenderer::CV_r_TexturesStreamingDebugMinSize * 2) ? Col_Red : Col_Green;

		// compute final mip factor
		bool bHighPriority = false;
		float fFinalMipFactor = pow(99.99f, 2.f);
		for (int z = 0; z < MAX_PREDICTION_ZONES; z++)
			if (tp->m_pFileTexMips && (int)tp->m_streamRounds[z].nRoundUpdateId > gRenDev->m_RP.m_TI[gRenDev->m_RP.m_nProcessThreadID].m_arrZonesRoundId[z] - 2)
			{
				fFinalMipFactor = min(fFinalMipFactor, tp->m_pFileTexMips->m_arrSPInfo[z].fLastMinMipFactor);
				bHighPriority |= tp->m_streamRounds[z].bLastHighPriority;
			}

		// how many times used in area around
		assert(tp->m_pFileTexMips);

		int nMipIdSigned = tp->StreamCalculateMipsSigned(fFinalMipFactor);

		if (nMipIdSigned > CRenderer::CV_r_TexturesStreamingDebugMinMip)
			continue;

		int nPersMip = tp->m_nMips - tp->m_CacheFileHeader.m_nMipsPersistent;
		int nMipReq = min(tp->GetRequiredMipNonVirtual(), nPersMip);
		const int nWantedSize = tp->StreamComputeDevDataSize(nMipReq);

		assert(tp->m_pFileTexMips);
		PREFAST_ASSUME(tp->m_pFileTexMips);
		cry_sprintf(szText, "%.2f | %.2f |%6.2f | %1d | %2d/%d/%d | %i/%i | %i | %s",
		            (float)tp->m_nActualSize / (1024 * 1024.0f),
		            (float)nWantedSize / (1024 * 1024.0f),
		            sqrtf(fFinalMipFactor),
		            (int)bHighPriority,
		            tp->GetNumMipsNonVirtual() - nMipIdSigned, tp->GetNumMipsNonVirtual() - tp->m_nMinMipVidUploaded, tp->GetNumMipsNonVirtual(),
		            tp->m_streamRounds[0].nRoundUpdateId, tp->m_streamRounds[MAX_PREDICTION_ZONES - 1].nRoundUpdateId,
		            tp->m_nAccessFrameID >= (int)ti.m_nFrameUpdateID - 8,
		            tp->m_SrcName.c_str());

		IRenderAuxText::WriteXY(nX, nY + (nTexNum + 1) * 10, 1.f, 1.f, color.r, color.g, color.b, 1, "%s", szText);
		if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
		{
			CryLogAlways("%s", szText);
		}

		++nTexNum;
		if (nTexNum > 50 && !CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
		{
			break;
		}
	}

	if (CRenderer::CV_r_TexturesStreamingDebugDumpIntoLog)
	{
		CryLogAlways("==============================================================================================================");
	}

	if (ICVar* pCVar = gEnv->pConsole->GetCVar("r_TexturesStreamingDebugDumpIntoLog"))
	{
		pCVar->Set(0);
	}
}
#endif
