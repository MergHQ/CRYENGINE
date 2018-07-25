// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CGpuBuffer;

namespace compute_skinning
{

#pragma pack(push)
#pragma pack(1)

struct SSkinning
{
	// packed as 8:24
	uint weightIndex;
};

struct SSkinningMap
{
	uint offset;
	uint count;
};

#pragma pack(pop)

struct IPerCharacterDataSupply
{
	//! Store weights and mapping
	//! At this point, weights are only added from render-thread - thus, resources are not locked atm
	virtual void RT_PushWeights(const int numWeights, const int numWeightsMap, const compute_skinning::SSkinning* weights, const compute_skinning::SSkinningMap* weightsMap) = 0;
};

struct IPerMeshDataSupply
{
	virtual void RT_PushMorphs(const int numMorphs, const int numMorphsBitField, const Vec4* morphsDeltas, const uint64* morphsBitField) = 0;
	virtual void RT_PushBindPoseBuffers(const int numVertices, const int numIndices, const int numAdjTriangles, const compute_skinning::SSkinVertexIn* vertices, const vtx_idx* indices, const uint32* adjTriangles) = 0;

	virtual std::shared_ptr<compute_skinning::IPerCharacterDataSupply> GetOrCreatePerCharacterResources(const uint32 guid) = 0;
};

struct IComputeSkinningStorage
{
	virtual std::shared_ptr<compute_skinning::IPerMeshDataSupply> GetOrCreatePerMeshResources(const CRenderMesh* pMesh) = 0;
	virtual CGpuBuffer*                                           GetOutputVertices(const void* pCustomTag) = 0;
};

}
