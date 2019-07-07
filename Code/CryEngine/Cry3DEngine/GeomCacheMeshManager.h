// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// ------------------------------------------------------------------------
//  Description: Manages static meshes for geometry caches
// -------------------------------------------------------------------------

#if defined(USE_GEOM_CACHES)

	#include <CryRenderer/IRenderMesh.h>
	#include <Cry3DEngine/GeomCacheFileFormat.h>
	#include "GeomCache.h"
	#include <CryCore/StlUtils.h>

class CGeomCacheMeshManager
{
public:
	void                    Reset();

	bool                    ReadMeshStaticData(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, SGeomCacheStaticMeshData& staticMeshData) const;
	_smart_ptr<IRenderMesh> ConstructStaticRenderMesh(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo,
	                                                  SGeomCacheStaticMeshData& staticMeshData, const char* pFileName);
	_smart_ptr<IRenderMesh> GetStaticRenderMesh(const uint64 hash) const;
	void                    RemoveReference(SGeomCacheStaticMeshData& staticMeshData);
private:
	bool                    ReadMeshIndices(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo,
	                                        SGeomCacheStaticMeshData& staticMeshData, std::vector<vtx_idx>& indices) const;
	bool                    ReadMeshPositions(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<Vec3> positions) const;
	bool                    ReadMeshTexcoords(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<Vec2> texcoords) const;
	bool                    ReadMeshQTangents(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<SPipTangents> tangents) const;
	bool                    ReadMeshColors(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<UCol> colors) const;

	struct SMeshMapInfo
	{
		_smart_ptr<IRenderMesh> m_pRenderMesh;
		uint                    m_refCount;
	};

	// Map from mesh hash to render mesh
	typedef std::unordered_map<uint64, SMeshMapInfo> TMeshMap;
	TMeshMap m_meshMap;
};

#endif
