// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Shadow_Renderer.h"

#include "RenderView.h"

#include "CompiledRenderObject.h"

#include <functional>
#include <vector>

static_assert(FOB_HAS_PREVMATRIX > FOB_ZPREPASS, "FOB-sort is in wrong order");
static_assert(FOB_ZPREPASS > FOB_ALPHATEST && FOB_ZPREPASS > FOB_DISSOLVE, "FOB-sort is in wrong order");
static_assert((FOB_SORT_MASK & ~FOB_DISCARD_MASK) & FOB_ZPREPASS, "FOB-sort mask doesn't contain zprepass");

///////////////////////////////////////////////////////////////////////////////
// sort operators for render items
struct SCompareItemPreprocess
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		uint64 sortValA(a.SortVal);
		uint64 sortValB(b.SortVal);
		uint64 batchFlagsA(a.nBatchFlags);
		uint64 batchFlagsB(b.nBatchFlags);

		uint64 sortAFusion = (batchFlagsA << 32) + (sortValA << 0);
		uint64 sortBFusion = (batchFlagsB << 32) + (sortValB << 0);

		if (sortAFusion != sortBFusion)
			return sortAFusion < sortBFusion;

		return a.rendItemSorter < b.rendItemSorter;
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareRendItem
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		// Sort by shaders (almost all the same)
		// NOTE: SortVal has arbitrary ordering
		// TODO: order by nearest distance per ID-bin
		if (a.SortVal != b.SortVal)
			return a.SortVal < b.SortVal;

		// Sorting by geometry, it's less important than sorting by shaders
		// NOTE: pElem has arbitrary ordering
		// TODO: order by nearest distance per Element-bin
		if (a.pElem != b.pElem)
			return a.pElem < b.pElem;

		// Sort by distance
		return (a.ObjSort & 0xFFFF) < (b.ObjSort & 0xFFFF);
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareRendItemZPrePass
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		// Cluster all alpha-testing shader-types together (needs texture, everything else not)
		uint32 nSortObjFlagsA = (a.ObjSort & (FOB_SORT_MASK & FOB_DISCARD_MASK));
		uint32 nSortObjFlagsB = (b.ObjSort & (FOB_SORT_MASK & FOB_DISCARD_MASK));
		if (nSortObjFlagsA != nSortObjFlagsB)
			return nSortObjFlagsA < nSortObjFlagsB;

		// Sort by depth/distance layers (pick half-float exponent, approximation to perspective Z)
		uint32 depthLayerA = (a.ObjSort >> 10) & 0x3F;
		uint32 depthLayerB = (b.ObjSort >> 10) & 0x3F;
		if (depthLayerA != depthLayerB)
			return depthLayerA < depthLayerB;

		// Sort by shaders (almost all the same)
		// NOTE: SortVal has arbitrary ordering
		// TODO: order by nearest distance per ID-bin
		if (a.SortVal != b.SortVal)
			return a.SortVal < b.SortVal;

		// Sorting by geometry, it's less important than sorting by shaders
		// NOTE: pElem has arbitrary ordering
		// TODO: order by nearest distance per Element-bin
		if (a.pElem != b.pElem)
			return a.pElem < b.pElem;

		// Sort by distance
		return (a.ObjSort & 0xFFFF) < (b.ObjSort & 0xFFFF);
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareRendItemZPass
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		// Bin/split list by VELOCITY first (see CSceneGBufferStage::ExecuteSceneOpaque)
		// Bin/split list by ZPREPASS afterwards
		// Cluster all discarding shader-types together (which are not already disabled because of zprepass)

		uint32 nSortObjFlagsA = (a.ObjSort & (FOB_SORT_MASK & ~FOB_DISCARD_MASK));
		uint32 nSortObjFlagsB = (b.ObjSort & (FOB_SORT_MASK & ~FOB_DISCARD_MASK));
		if (nSortObjFlagsA != nSortObjFlagsB)
			return nSortObjFlagsA < nSortObjFlagsB;

		// Sort by depth/distance layers (pick half-float exponent, approximation to perspective Z)
		uint32 depthLayerA = !(nSortObjFlagsA & FOB_ZPREPASS) ? (a.ObjSort >> 10) & 0x3F : 0x3F;
		uint32 depthLayerB = !(nSortObjFlagsB & FOB_ZPREPASS) ? (b.ObjSort >> 10) & 0x3F : 0x3F;
		if (depthLayerA != depthLayerB)
			return depthLayerA < depthLayerB;

		// Sort by shaders
		// NOTE: SortVal has arbitrary ordering
		// TODO: order by nearest distance per ID-bin
		if (a.SortVal != b.SortVal)
			return a.SortVal < b.SortVal;

		// Sorting by geometry, it's less important than sorting by shaders
		// NOTE: pElem has arbitrary ordering
		// TODO: order by nearest distance per Element-bin
		if (a.pElem != b.pElem)
			return a.pElem < b.pElem;

		// Sort by distance
		return (a.ObjSort & 0xFFFF) < (b.ObjSort & 0xFFFF);
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareItem_Decal
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		uint64 sortValA(a.SortVal);
		uint64 sortValB(b.SortVal);
		uint64 objSortA_Distance(a.ObjSort & 0xFFFF);
		uint64 objSortB_Distance(b.ObjSort & 0xFFFF);
		uint64 objSortA_ObjFlags(a.ObjSort & ~0xFFFF);
		uint64 objSortB_ObjFlags(b.ObjSort & ~0xFFFF);

		uint64 objSortAFusion = (objSortA_Distance << 48) + (sortValA << 16) + (objSortA_ObjFlags << 0);
		uint64 objSortBFusion = (objSortB_Distance << 48) + (sortValB << 16) + (objSortB_ObjFlags << 0);

		return objSortAFusion < objSortBFusion;
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareItem_NoPtrCompare
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.ObjSort != b.ObjSort)
			return a.ObjSort < b.ObjSort;

		float pSurfTypeA = ((float*)a.pElem->m_CustomData)[8];
		float pSurfTypeB = ((float*)b.pElem->m_CustomData)[8];
		if (pSurfTypeA != pSurfTypeB)
			return (pSurfTypeA < pSurfTypeB);

		pSurfTypeA = ((float*)a.pElem->m_CustomData)[9];
		pSurfTypeB = ((float*)b.pElem->m_CustomData)[9];
		if (pSurfTypeA != pSurfTypeB)
			return (pSurfTypeA < pSurfTypeB);

		pSurfTypeA = ((float*)a.pElem->m_CustomData)[11];
		pSurfTypeB = ((float*)b.pElem->m_CustomData)[11];
		return (pSurfTypeA < pSurfTypeB);
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareDist
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.fDist == b.fDist)
			return a.rendItemSorter.ParticleCounter() < b.rendItemSorter.ParticleCounter();

		return (a.fDist > b.fDist);
	}
};

///////////////////////////////////////////////////////////////////////////////
struct SCompareDistInverted
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.fDist == b.fDist)
			return a.rendItemSorter.ParticleCounter() > b.rendItemSorter.ParticleCounter();

		return (a.fDist < b.fDist);
	}
};

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortPreprocess(SRendItem* First, int Num)
{
	std::sort(First, First + Num, SCompareItemPreprocess());
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortForZPass(SRendItem* First, int Num, bool bZPre)
{
	if (bZPre)
		std::sort(First, First + Num, SCompareRendItemZPrePass());
	else
		std::sort(First, First + Num, SCompareRendItemZPass());
}

void SRendItem::mfSortForDepthPass(SRendItem* First, int Num)
{
	std::sort(First, First + Num, SCompareRendItemZPrePass());
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortByLight(SRendItem* First, int Num, bool bSort, const bool bIgnoreRePtr, bool bSortDecals)
{
	if (bSort)
	{
		if (bIgnoreRePtr)
			std::sort(First, First + Num, SCompareItem_NoPtrCompare());
		else if (bSortDecals)
			std::sort(First, First + Num, SCompareItem_Decal());
		else
			std::sort(First, First + Num, SCompareRendItem());
	}
}

//////////////////////////////////////////////////////////////////////////
void SRendItem::mfSortByDist(SRendItem* First, int Num, bool bDecals, bool InvertedOrder)
{
	//Note: Temporary use stable sort for flickering hair (meshes within the same skin attachment don't have a deterministic sort order)
	if (bDecals)
		std::stable_sort(First, First + Num, SCompareItem_Decal());
	else if (InvertedOrder)
		std::stable_sort(First, First + Num, SCompareDistInverted());
	else
		std::stable_sort(First, First + Num, SCompareDist());
}
