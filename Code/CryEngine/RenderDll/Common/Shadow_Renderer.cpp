// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Shadow_Renderer.h"

#include "D3D_SVO.h"

#include "Common/RenderView.h"
#include "GraphicsPipeline/TiledShading.h"

void ShadowMapFrustum::SortRenderItemsForFrustumAsync(int side, SRendItem* pFirst, size_t nNumRendItems)
{
	FUNCTION_PROFILER_RENDERER();

	SRendItem::mfSortForDepthPass(pFirst, nNumRendItems);
}

//////////////////////////////////////////////////////////////////////////
CRenderView* ShadowMapFrustum::GetNextAvailableShadowsView(CRenderView* pMainRenderView)
{
	CRenderView* pShadowsView = gcpRendD3D->GetOrCreateRenderView(CRenderView::eViewType_Shadow);
	CRY_ASSERT(pShadowsView->GetUsageMode() == IRenderView::eUsageModeUndefined || pShadowsView->GetUsageMode() == IRenderView::eUsageModeReadingDone);

	pShadowsView->SetParentView(pMainRenderView);
	pShadowsView->SetFrameId(pMainRenderView->GetFrameId());
	pShadowsView->SetSkinningDataPools(pMainRenderView->GetSkinningDataPools());

	CRY_ASSERT(!((CRenderView*)pShadowsView)->CheckPermanentRenderObjects(), "There shouldn't be any permanent render-object be present in the view!");

	return pShadowsView;
}

IRenderView* CRenderer::GetNextAvailableShadowsView(IRenderView* pMainRenderView, ShadowMapFrustum* pOwnerFrustum)
{
	return pOwnerFrustum->GetNextAvailableShadowsView((CRenderView*)pMainRenderView);
}

//////////////////////////////////////////////////////////////////////////
uint32 ShadowMapFrustum::GenerateTimeSlicedUpdateCacheMask(uint32 currentFrame, uint32 *timeSlicedShadowsUpdated)
{
	CRY_ASSERT(!bRestrictToRT);

	uint32 frustumSideUpdateMask = UpdateRequests();
	uint32 frustumSideCacheMask = 0;
	auto timeSlicedShadowUpdatesLimit = static_cast<uint32>(CRenderer::CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame);

	// For shadow pools, respect CV_r_ShadowPoolMaxTimeslicedUpdatesPerFrame and nShadowPoolUpdateRate
	if (const auto shadowPoolUpdateRate = min<uint8>(CRenderer::CV_r_ShadowPoolMaxFrames, nShadowPoolUpdateRate))
	{
		CRY_ASSERT(HasVariableUpdateRate());

		for (auto nSide = 0; nSide < GetNumSides(); ++nSide)
		{
			if (!(frustumSideUpdateMask & BIT(nSide)))
				continue;

			uint32 previousFrame = this->nSideDrawnOnFrame[nSide];
			uint32 deltaFrames = currentFrame - previousFrame;

			// Can only cache if there's content from before
			if (previousFrame > 0)
			{
				if (deltaFrames < shadowPoolUpdateRate)
				{
					frustumSideCacheMask |= BIT(nSide);
				}

				// Limit time-sliced updates
				else if (timeSlicedShadowsUpdated)
				{
					if (*timeSlicedShadowsUpdated >= timeSlicedShadowUpdatesLimit)
						frustumSideCacheMask |= BIT(nSide);
					else
						++*timeSlicedShadowsUpdated;
				}
			}
		}
	}
	
	return frustumSideCacheMask;
}

//////////////////////////////////////////////////////////////////////////
uint32 ShadowMapFrustum::AssignShadowPoolLocations(uint32& numShadowPoolAllocsThisFrame, CPowerOf2BlockPacker& blockPacker, TArray<SShadowAllocData>& shadowPoolAlloc, const SRenderLight& light)
{
	CRY_ASSERT(!bRestrictToRT);

	uint32 frustumSideRenderMask = UpdateRequests();
	uint32 frustumSideRedoMask = 0;
	const auto lightID = light.m_nEntityId;
	int nBlockW = nTexSize >> TEX_POOL_BLOCKLOGSIZE;
	int nLogBlockW = IntegerLog2((uint32)nBlockW);
	int nLogBlockH = nLogBlockW;

	CRY_ASSERT(lightID != (uint32)-1);

	// Reserve a shadowpool slot for each side
	for (uint nSide = 0; nSide < GetNumSides(); ++nSide)
	{
		if (!(frustumSideRenderMask & BIT(nSide)))
			continue;

		TRect_tpl<uint32> poolPack;

		SShadowAllocData* pAlloc = nullptr;
		for (SShadowAllocData& e : shadowPoolAlloc)
		{
			if (e.m_lightID == lightID && e.m_side == nSide)
			{
				pAlloc = &e;
				break;
			}
		}

		if (pAlloc)
		{
			const auto nID = blockPacker.GetBlockInfo(pAlloc->m_blockID, poolPack.Min.x, poolPack.Min.y, poolPack.Max.x, poolPack.Max.y);
			if (nID != 0xFFFFFFFF)
			{
				if (poolPack.Min.x != 0xFFFFFFFF)
				{
					if (nBlockW == poolPack.Max.x - poolPack.Min.x)   // ignore Y, is square
					{
						blockPacker.GetBlockInfo(nID, poolPack.Min.x, poolPack.Min.y, poolPack.Max.x, poolPack.Max.y);

						poolPack.Min.x <<= TEX_POOL_BLOCKLOGSIZE;
						poolPack.Min.y <<= TEX_POOL_BLOCKLOGSIZE;
						poolPack.Max.x <<= TEX_POOL_BLOCKLOGSIZE;
						poolPack.Max.y <<= TEX_POOL_BLOCKLOGSIZE;

						if (shadowPoolPack[nSide] != poolPack)
							frustumSideRedoMask |= BIT(nSide);
						shadowPoolPack[nSide] = poolPack;

						continue; // All currently valid, skip
					}

					blockPacker.RemoveBlock(nID);
					pAlloc->Clear();
				}
			}
		}

		// Try to allocate new block
#if defined(ENABLE_PROFILING_CODE)
		++numShadowPoolAllocsThisFrame;
#endif
		const auto nID = blockPacker.AddBlock(nLogBlockW, nLogBlockH);
		if (nID != 0xFFFFFFFF)
		{
			if (!pAlloc)
			{
				// Attempt to find a free block
				for (SShadowAllocData& e : shadowPoolAlloc)
				{
					if (e.isFree())
					{
						pAlloc = &e;
						break;
					}
				}

				// Otherwise create a new one
				if (!pAlloc)
					pAlloc = shadowPoolAlloc.AddIndex(1);
			}

			pAlloc->m_blockID = nID;
			pAlloc->m_lightID = lightID;
			pAlloc->m_side    = nSide;

			blockPacker.GetBlockInfo(nID, poolPack.Min.x, poolPack.Min.y, poolPack.Max.x, poolPack.Max.y);

			poolPack.Min.x <<= TEX_POOL_BLOCKLOGSIZE;
			poolPack.Min.y <<= TEX_POOL_BLOCKLOGSIZE;
			poolPack.Max.x <<= TEX_POOL_BLOCKLOGSIZE;
			poolPack.Max.y <<= TEX_POOL_BLOCKLOGSIZE;

			if (shadowPoolPack[nSide] != poolPack)
				frustumSideRedoMask |= BIT(nSide);
			shadowPoolPack[nSide] = poolPack;
		}
		else
		{
			// InvalidateSide(nSide);
			// abort();

#if defined(ENABLE_PROFILING_CODE)
			if (CRenderer::CV_r_ShadowPoolMaxFrames != 0 || CRenderer::CV_r_DeferredShadingTiled >= CTiledShadingStage::eDeferredMode_1Pass)
				numShadowPoolAllocsThisFrame |= 0x80000000;
#endif
		}
	}

	return frustumSideRedoMask;
}

//////////////////////////////////////////////////////////////////////////
uint32 CRenderer::PrepareShadowFrustumForShadowPool(IRenderView* pMainRenderView, ShadowMapFrustum* pFrustum, const SRenderLight& light, uint32 frameID, uint32 *timeSlicedShadowsUpdated)
{
	CRY_ASSERT(!pFrustum->bRestrictToRT);

	uint32 frustumSideUpdateMask = pFrustum->UpdateRequests();
	if (!pFrustum->bUseShadowsPool)
	{
		return frustumSideUpdateMask;
	}
	
#if defined(ENABLE_PROFILING_CODE)
	uint32& numShadowPoolAllocsThisFrame = m_frameRenderStats[m_nFillThreadID].m_NumShadowPoolAllocsThisFrame;
#else
	uint32  numShadowPoolAllocsThisFrame = 0;
#endif

	CDeferredShading* pDeferredShading = pMainRenderView->GetGraphicsPipeline()->GetDeferredShading();

	// Remove update-requests for delayed shadow-updates (i.e. update-rate)
	frustumSideUpdateMask &= ~pFrustum->GenerateTimeSlicedUpdateCacheMask(frameID, timeSlicedShadowsUpdated);

	// Add back in requests for first-time allocations and block-moves (size-change, defragmentation, etc. pp.)
	frustumSideUpdateMask |=  pFrustum->AssignShadowPoolLocations(
		numShadowPoolAllocsThisFrame,
		pDeferredShading->m_blockPack,
		pDeferredShading->m_shadowPoolAlloc,
		light);

	return frustumSideUpdateMask;
}
