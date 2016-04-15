// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __GRAPHNODEMANAGER_H__
#define __GRAPHNODEMANAGER_H__

#include "GraphStructures.h"

class CGraphNodeManager
{
	enum {BUCKET_SIZE_SHIFT = 7};
	enum {BUCKET_SIZE = 128};

public:
	CGraphNodeManager();
	~CGraphNodeManager();

	// NOTE Oct 20, 2009: <pvl> only called from Graph::Clear() and own destructor
	void       Clear(IAISystem::tNavCapMask typeMask);

	unsigned   CreateNode(IAISystem::tNavCapMask type, const Vec3& pos, unsigned ID);
	void       DestroyNode(unsigned index);

	GraphNode* GetNode(unsigned index)
	{
		if (!index)
			return 0;
		int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
		// FIXME Oct 20, 2009: <pvl> we need to be able to return 0 here (meaning
		// the node was likely unloaded) => get rid of the GetDummyNode() hack
		if (!m_buckets[bucket])
			return GetDummyNode();
		return reinterpret_cast<GraphNode*>(m_buckets[bucket]->nodes +
		                                    (((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->GetNodeSize()));
	}

	const GraphNode* GetNode(unsigned index) const
	{
		if (!index)
			return 0;
		int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
		// FIXME Oct 20, 2009: <pvl> we need to be able to return 0 here (meaning
		// the node was likely unloaded) => get rid of the GetDummyNode() hack
		if (!m_buckets[bucket])
			return GetDummyNode();
		return reinterpret_cast<GraphNode*>(m_buckets[bucket]->nodes +
		                                    (((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->GetNodeSize()));
	}

	size_t NodeMemorySize() const;
	void   GetMemoryStatistics(ICrySizer* pSizer);

private:
	class BucketHeader
	{
	public:
		enum { InvalidNextFreeBucketIdx = 0xffff };
		enum { InvalidNextAvailableIdx = 0xff };

	public:
		unsigned type;
		uint16   nextFreeBucketIdx;
		uint8    nodeSizeS2;
		uint8    nextAvailable;
		uint8*   nodes;

		ILINE size_t GetNodeSize() const    { return (size_t)nodeSizeS2 << 2; }
		ILINE void   SetNodeSize(size_t sz) { nodeSizeS2 = static_cast<uint8>(sz >> 2); }
	};

	// This function exists only to hide some fundamental problems in the path-finding system. Basically
	// things aren't cleaned up properly as of writing (pre-alpha Crysis 5/5/2007). Therefore we return
	// an object that looks like a graph node to stop client code from barfing.
	GraphNode* GetDummyNode() const
	{
		static char buffer[sizeof(GraphNode) + 100] = { 0 };
		static GraphNode* pDummy = new(buffer) GraphNode_Triangular(IAISystem::NAV_TRIANGULAR, Vec3(0, 0, 0), 0xCECECECE);
		return pDummy;
	}

	int TypeSizeFromTypeIndex(unsigned typeIndex) const;

	std::vector<BucketHeader*> m_buckets;
	std::vector<uint16>        m_freeBuckets;
	int                        m_typeSizes[IAISystem::NAV_TYPE_COUNT];
};

#endif //__GRAPHNODEMANAGER_H__
