// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQuery.h>

#include "Tile.h"

#ifdef NAV_MESH_QUERY_DEBUG
#include "NavMeshQueryDebug.h"
#include <CryUDR/InterfaceIncludes.h>
#endif // NAV_MESH_QUERY_DEBUG

namespace MNM
{
class CNavMesh;

namespace DefaultQueryFilters
{
	//! Filter for QueryTriangles, which accepts all triangles.
	//! Note, that the signature of PassFilter(), GetCostMultiplier() and GetCost() functions is same as in INavMeshQueryFilter, but it's not virtual.
	//! This way, templated implementation functions can avoid unnecessary checks and virtual calls.
	struct SAcceptAllQueryTrianglesFilter
	{
		inline bool PassFilter(const Tile::STriangle&) const { return true; }
		inline float GetCostMultiplier(const MNM::Tile::STriangle& triangle) const { return 1.0f; }
	};
	
	extern const SAcceptAllQueryTrianglesFilter g_acceptAllTriangles;
}

class CTriangleAtQuery : public INavMeshQueryProcessing
{
public:
	CTriangleAtQuery(const CNavMesh* pNavMesh, const vector3_t& localPosition)
		: m_pNavMesh(pNavMesh)
		, m_location(localPosition)
	{}

	const MNM::TriangleID GetTriangleId() const { return m_closestID; }

	virtual EResult operator() (const MNM::TriangleIDArray& triangleIDArray) override;

private:
	const CNavMesh* m_pNavMesh;

	vector3_t m_location;
	TriangleID m_closestID = MNM::Constants::InvalidTriangleID;
	MNM::real_t::unsigned_overflow_type m_distMinSq = std::numeric_limits<MNM::real_t::unsigned_overflow_type>::max();
};

class CNearestTriangleQuery : public INavMeshQueryProcessing
{
public:
	CNearestTriangleQuery(const CNavMesh* pNavMesh, const vector3_t& localFromPosition, const MNM::real_t maxDistance = real_t::max())
		: m_pNavMesh(pNavMesh)
		, m_localFromPosition(localFromPosition)
		, m_distMinSq(maxDistance.sqr())
	{}

	const vector3_t& GetClosestPosition() const { return m_closestPos; }
	const real_t GetClosestDistance() const { return real_t::sqrtf(m_distMinSq); }
	const MNM::TriangleID GetClosestTriangleId() const { return m_closestID; }

	virtual EResult operator() (const MNM::TriangleIDArray& triangleIDArray) override;

private:
	const CNavMesh* m_pNavMesh;
	const vector3_t m_localFromPosition;

	TriangleID m_closestID = MNM::Constants::InvalidTriangleID;
	vector3_t m_closestPos = real_t::max();
	MNM::real_t::unsigned_overflow_type m_distMinSq = std::numeric_limits<MNM::real_t::unsigned_overflow_type>::max();
};

class CGetMeshBordersQueryWithFilter : public INavMeshQueryProcessing
{
public:
	CGetMeshBordersQueryWithFilter(const CNavMesh* pNavMesh, Vec3* pBordersWithNormalsArray, const size_t maxBordersCount, const INavMeshQueryFilter& filter)
		: m_pNavMesh(pNavMesh)
		, m_pBordersWithNormalsArray(pBordersWithNormalsArray)
		, m_maxBordersCount(maxBordersCount)
		, m_filter(filter)
		, m_bordersCount(0)
	{}

	const size_t GetFoundBordersCount() const { return m_bordersCount; }

	virtual EResult operator() (const MNM::TriangleIDArray& triangleIDArray) override;

private:
	const CNavMesh* m_pNavMesh;
	const vector3_t m_localFromPosition;
	const size_t m_maxBordersCount;
	const INavMeshQueryFilter& m_filter;
	
	Vec3* m_pBordersWithNormalsArray;
	size_t m_bordersCount;
};

class CGetMeshBordersQueryNoFilter : public INavMeshQueryProcessing
{
public:
	CGetMeshBordersQueryNoFilter(const CNavMesh* pNavMesh, Vec3* pBordersWithNormalsArray, const size_t maxBordersCount)
		: m_pNavMesh(pNavMesh)
		, m_pBordersWithNormalsArray(pBordersWithNormalsArray)
		, m_maxBordersCount(maxBordersCount)
		, m_bordersCount(0)
	{}

	const size_t GetFoundBordersCount() const { return m_bordersCount; }

	virtual EResult operator() (const MNM::TriangleIDArray& triangleIDArray) override;

private:
	const CNavMesh* m_pNavMesh;
	const vector3_t m_localFromPosition;
	const size_t m_maxBordersCount;

	Vec3* m_pBordersWithNormalsArray;
	size_t m_bordersCount;
};

YASLI_ENUM_BEGIN_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EActionOnNavMeshChange, "EActionOnNavMeshChange")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EActionOnNavMeshChange::Ignore, "Ignore")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EActionOnNavMeshChange::InvalidateAlways, "InvalidateAlways")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EActionOnNavMeshChange::InvalidateIfProccesed, "InvalidateIfProccesed")
YASLI_ENUM_END()

YASLI_ENUM_BEGIN_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EOverlappingMode, "EOverlappingMode")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EOverlappingMode::BoundingBox_Partial, "BoundingBox_Partial")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EOverlappingMode::BoundingBox_Full, "BoundingBox_Full")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EOverlappingMode::Triangle_Partial, "Triangle_Partial")
YASLI_ENUM_VALUE_NESTED2(INavMeshQuery, SNavMeshQueryConfig, EOverlappingMode::Triangle_Full, "Triangle_Full")
YASLI_ENUM_END()

class CNavMeshQueryManager;

class CNavMeshQuery : public INavMeshQuery
{
	typedef std::unordered_set<TileID> TileIdSet;

	friend class CNavMeshQueryManager;

	struct RuntimeData
	{
		RuntimeData()
			: pMesh(nullptr)
			, meshId(0)
			, tileMinX(0)
			, tileMinY(0)
			, tileMinZ(0)
			, tileMaxX(0)
			, tileMaxY(0)
			, tileMaxZ(0)
			, lastTileX(0)
			, lastTileY(0)
			, lastTileZ(0)
			, currentTileCompleted(false)
			, stoppedAtTriangleIndexInIncompleteTile(0)
			, remainingTrianglesBudgetInBatch(0)
		{}

		const CNavMesh* pMesh;
		NavigationMeshID meshId;

		// Tile data 
		size_t tileMinX;
		size_t tileMinY;
		size_t tileMinZ;

		size_t tileMaxX;
		size_t tileMaxY;
		size_t tileMaxZ;

		size_t lastTileX;
		size_t lastTileY;
		size_t lastTileZ;

		bool currentTileCompleted;
		size_t stoppedAtTriangleIndexInIncompleteTile;
		size_t remainingTrianglesBudgetInBatch;
	};

public:
	~CNavMeshQuery();

	virtual NavMeshQueryId                               GetId() const override final;
	virtual const SNavMeshQueryConfig&                   GetQueryConfig() const override final;
	virtual INavMeshQuery::EQueryStatus                  GetStatus() const override final;
	virtual bool                                         HasInvalidStatus() const override final;

	virtual INavMeshQuery::EQueryStatus                  PullTrianglesBatch(const size_t batchCount, TriangleIDArray& outTriangles) override final;

#ifdef NAV_MESH_QUERY_DEBUG
	virtual const INavMeshQueryDebug&                    GetQueryDebug() const override final;
#endif // NAV_MESH_QUERY_DEBUG
private:
	CNavMeshQuery(const NavMeshQueryId queryId, const SNavMeshQueryConfig& queryConfig);

	void                                                 OnNavMeshChanged(NavigationAgentTypeID navAgentId, NavigationMeshID navMeshId, TileID tileId);
	void                                                 OnNavMeshAnnotationChanged(NavigationAgentTypeID navAgentId, NavigationMeshID navMeshId, TileID tileId);

	void                                                 RegisterNavMeshCallbacks();
	void                                                 DeregisterNavMeshCallbacks();

	bool                                                 InitializeRuntimeData();
	bool                                                 AllTilesDone() const;
	bool                                                 IsTileProcessed(const TileID tileId) const;

	void                                                 SetTileProcessed(const TileID tileId);
	void                                                 SetStatus(const INavMeshQuery::EQueryStatus status);
	INavMeshQuery::EQueryStatus                          PullNextTriangleBatchInternal(const size_t batchCount, TriangleIDArray& outTriangles);

	bool                                                 SetupNavMeshInRuntimeData();

	void                                                 ProcessTilesTriangles(const size_t batchCount, TriangleIDArray& outTriangles, size_t& outTrianglesCount);

	bool                                                 ProcessTileTriangles(const TileID tileId, const size_t batchCount, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const;

	INavMeshQueryProcessing::EResult                     TryApplyNavMeshQueryProcessing(const TriangleIDArray& triangleIDArray);

	template<typename TFilter>
	bool                                                 QueryTileTriangles(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const;

	template<typename TFilter>
	bool                                                 QueryTileTrianglesBV(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const;

	template<typename TFilter>
	bool                                                 QueryTileTrianglesLinear(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outResumeTriangleIndexInTile) const;

	bool                                                 TrianglesBBoxIntersectionBV(const aabb_t& queryAabbTile, const aabb_t& nodeAabb_, const TileID tileId, const TriangleID triangleId) const;

	bool                                                 TrianglesBBoxIntersectionLinear(const aabb_t& queryAabbTile, const vector3_t& v0, const vector3_t& v1, const vector3_t& v2) const;

	bool                                                 TrianglesBBoxIntersectionHelper(const aabb_t& queryAabbTile, const aabb_t& triangleAabb, const vector3_t& v0, const vector3_t& v1, const vector3_t& v2) const;

#ifdef NAV_MESH_QUERY_DEBUG
	void                                                 DebugDrawQueryConfig() const;
	void                                                 DebugDrawQueryBatch(const INavMeshQueryDebug::SBatchData& queryBatch) const;               
	void                                                 DebugDrawQueryInvalidation(const INavMeshQueryDebug::SInvalidationData& queryInvalidation) const;

	const CNavMesh*                                      GetQueryEnclosingMesh(const INavMeshQuery::SNavMeshQueryConfig& queryConfig) const;
	void                                                 DebugDrawTriangleUDR(const TriangleID triangleID, const vector3_t v0, const vector3_t v1, const vector3_t v2, Cry::UDR::CScopeBase& scope, const ColorF& triangleColor) const;

#endif // NAV_MESH_QUERY_DEBUG

	const NavMeshQueryId                                 m_queryId;
	const SNavMeshQueryConfig                            m_queryConfig;
	EQueryStatus                                         m_status;
	TileIdSet                                            m_processedTileIdSet;
	RuntimeData                                          m_runtimeData;

#ifdef NAV_MESH_QUERY_DEBUG
	size_t                                               m_batchCount;
	CNavMeshQueryDebug                                   m_queryDebug;
#endif // NAV_MESH_QUERY_DEBUG
};

DECLARE_SHARED_POINTERS(CNavMeshQuery);
} // namespace MNM