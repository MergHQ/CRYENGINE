// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "NavMesh.h"
#include "NavMeshQuery.h"
#include "../NavigationSystem/NavigationSystem.h"

namespace MNM
{

namespace DefaultQueryFilters
{
	const SAcceptAllQueryTrianglesFilter g_acceptAllTriangles;
}

INavMeshQueryProcessing::EResult CTriangleAtQuery::operator() (const MNM::TriangleIDArray& triangleIDArray)
{
	MNM::vector3_t a, b, c;

	for (size_t i = 0; i < triangleIDArray.size(); ++i)
	{
		m_pNavMesh->GetVertices(triangleIDArray[i], a, b, c);

		if (PointInTriangle(vector2_t(m_location), vector2_t(a), vector2_t(b), vector2_t(c)))
		{
			const MNM::vector3_t ptClosest = ClosestPtPointTriangle(m_location, a, b, c);
			const MNM::real_t::unsigned_overflow_type dSq = (ptClosest - m_location).lenSqNoOverflow();

			if (dSq < m_distMinSq)
			{
				m_distMinSq = dSq;
				m_closestID = triangleIDArray[i];
			}
		}
	}
	return EResult::Continue;
}

INavMeshQueryProcessing::EResult CNearestTriangleQuery::operator() (const MNM::TriangleIDArray& triangleIDArray)
{
	MNM::vector3_t a, b, c;

	for (size_t i = 0; i < triangleIDArray.size(); ++i)
	{
		const MNM::TriangleID triangleId = triangleIDArray[i];

		if (m_pNavMesh->GetVertices(triangleId, a, b, c))
		{
			const MNM::vector3_t ptClosest = ClosestPtPointTriangle(m_localFromPosition, a, b, c);
			const MNM::real_t::unsigned_overflow_type dSq = (ptClosest - m_localFromPosition).lenSqNoOverflow();

			if (dSq < m_distMinSq)
			{
				m_closestID = triangleId;
				m_distMinSq = dSq;
				m_closestPos = ptClosest;
			}
		}
	}
	return EResult::Continue;
}

INavMeshQueryProcessing::EResult CGetMeshBordersQueryWithFilter::operator() (const MNM::TriangleIDArray& triangleIDArray)
{
	for (size_t i = 0; i < triangleIDArray.size(); ++i)
	{
		const MNM::TriangleID triangleId = triangleIDArray[i];
		const MNM::TileID tileId = MNM::ComputeTileID(triangleId);
		const MNM::STile& tile = m_pNavMesh->GetTile(tileId);

		Tile::STriangle triangle;
		m_pNavMesh->GetTriangle(triangleId, triangle);

		size_t linkedEdges = 0;
		for (size_t l = 0; l < triangle.linkCount; ++l)
		{
			const Tile::SLink& link = tile.GetLinks()[triangle.firstLink + l];
			const size_t edge = link.edge;

			if (link.side == Tile::SLink::Internal)
			{
				const MNM::AreaAnnotation* annotation = m_pNavMesh->GetTriangleAnnotation(ComputeTriangleID(tileId, link.triangle));
				if (m_filter.PassFilter(*annotation))
				{
					linkedEdges |= static_cast<size_t>(1) << edge;
				}
			}
			else if (link.side != Tile::SLink::OffMesh)
			{
				TileID neighbourTileID = m_pNavMesh->GetNeighbourTileID(tileId, link.side);
				const MNM::AreaAnnotation* annotation = m_pNavMesh->GetTriangleAnnotation(ComputeTriangleID(neighbourTileID, link.triangle));
				if (m_filter.PassFilter(*annotation))
				{
					linkedEdges |= static_cast<size_t>(1) << edge;
				}
			}
		}

		if (linkedEdges == 7)
			continue;

		vector3_t verts[3];
		m_pNavMesh->GetVertices(triangleId, verts);

		for (size_t e = 0; e < 3; ++e)
		{
			if ((linkedEdges & (size_t(1) << e)) == 0)
			{
				if (m_pBordersWithNormalsArray != nullptr)
				{
					const Vec3 v0 = verts[e].GetVec3();
					const Vec3 v1 = verts[(e + 1) % 3].GetVec3();
					const Vec3 vOther = verts[(e + 2) % 3].GetVec3();

					const Vec3 edge = Vec3(v0 - v1).GetNormalized();
					const Vec3 otherEdge = Vec3(v0 - vOther).GetNormalized();
					const Vec3 up = edge.Cross(otherEdge);
					const Vec3 normalOut = up.Cross(edge);

					m_pBordersWithNormalsArray[m_bordersCount * 3 + 0] = v0;
					m_pBordersWithNormalsArray[m_bordersCount * 3 + 1] = v1;
					m_pBordersWithNormalsArray[m_bordersCount * 3 + 2] = normalOut;

					if (++m_bordersCount == m_maxBordersCount)
						return EResult::Stop;
				}
				else
				{
					++m_bordersCount;
				}
			}
		}
	}
	return EResult::Continue;
}

INavMeshQueryProcessing::EResult CGetMeshBordersQueryNoFilter::operator() (const MNM::TriangleIDArray& triangleIDArray)
{
	for (size_t i = 0; i < triangleIDArray.size(); ++i)
	{
		const MNM::TriangleID triangleId = triangleIDArray[i];
		const MNM::TileID tileId = MNM::ComputeTileID(triangleId);
		const MNM::STile& tile = m_pNavMesh->GetTile(tileId);

		Tile::STriangle triangle;
		m_pNavMesh->GetTriangle(triangleId, triangle);

		size_t linkedEdges = 0;
		for (size_t l = 0; l < triangle.linkCount; ++l)
		{
			const Tile::SLink& link = tile.GetLinks()[triangle.firstLink + l];
			const size_t edge = link.edge;
			linkedEdges |= static_cast<size_t>(1) << edge;
		}

		if (linkedEdges == 7)
			continue;

		vector3_t verts[3];
		m_pNavMesh->GetVertices(triangleId, verts);

		for (size_t e = 0; e < 3; ++e)
		{
			if ((linkedEdges & (size_t(1) << e)) == 0)
			{
				if (m_pBordersWithNormalsArray != nullptr)
				{
					const Vec3 v0 = verts[e].GetVec3();
					const Vec3 v1 = verts[(e + 1) % 3].GetVec3();
					const Vec3 vOther = verts[(e + 2) % 3].GetVec3();

					const Vec3 edge = Vec3(v0 - v1).GetNormalized();
					const Vec3 otherEdge = Vec3(v0 - vOther).GetNormalized();
					const Vec3 up = edge.Cross(otherEdge);
					const Vec3 out = up.Cross(edge);

					m_pBordersWithNormalsArray[m_bordersCount * 3 + 0] = v0;
					m_pBordersWithNormalsArray[m_bordersCount * 3 + 1] = v1;
					m_pBordersWithNormalsArray[m_bordersCount * 3 + 2] = out;

					if (++m_bordersCount == m_maxBordersCount)
						return EResult::Stop;
				}
				else
				{
					++m_bordersCount;
				}
			}
		}
	}
	return EResult::Continue;
}

//====================================================================
// CNavMeshQuery
//====================================================================

CNavMeshQuery::~CNavMeshQuery()
{
	DeregisterNavMeshCallbacks();
}

NavMeshQueryId CNavMeshQuery::GetId() const
{
	return m_queryId;
}

const INavMeshQuery::SNavMeshQueryConfig& CNavMeshQuery::GetQueryConfig() const
{
	return m_queryConfig;
}

INavMeshQuery::EQueryStatus CNavMeshQuery::GetStatus() const
{
	return m_status;
}

bool CNavMeshQuery::HasInvalidStatus() const
{
	return m_status == MNM::INavMeshQuery::EQueryStatus::InvalidDueToNavMeshAnnotationChanges || 
		m_status == MNM::INavMeshQuery::EQueryStatus::InvalidDueToNavMeshRegeneration || 
		m_status == MNM::INavMeshQuery::EQueryStatus::InvalidDueToCachedNavMeshIdNotFound;
}

INavMeshQuery::EQueryStatus CNavMeshQuery::PullTrianglesBatch(const size_t batchSize, TriangleIDArray& outTriangles)
{
	CRY_PROFILE_FUNCTION(PROFILE_AI);

	switch (m_status)
	{
	case INavMeshQuery::EQueryStatus::Done:
	case INavMeshQuery::EQueryStatus::InvalidDueToNavMeshRegeneration:
	case INavMeshQuery::EQueryStatus::InvalidDueToNavMeshAnnotationChanges:
	case INavMeshQuery::EQueryStatus::InvalidDueToCachedNavMeshIdNotFound:
		break;
	case INavMeshQuery::EQueryStatus::Running:
		SetStatus(PullNextTriangleBatchInternal(batchSize, outTriangles));
		break;
	default:
		CRY_ASSERT_MESSAGE(false, "New enum value has been added to MNM::INavMeshQuery::EQueryStatus but it's not being handled in this switch statement");
	}
	return m_status;
}

#ifdef NAV_MESH_QUERY_DEBUG
const INavMeshQueryDebug& CNavMeshQuery::GetQueryDebug() const
{
	return m_queryDebug;
}
#endif // NAV_MESH_QUERY_DEBUG

CNavMeshQuery::CNavMeshQuery(const NavMeshQueryId queryId, const SNavMeshQueryConfig& queryConfig)
	: m_queryId(queryId)
	, m_queryConfig(queryConfig)
	, m_status(INavMeshQuery::EQueryStatus::Running)
#ifdef NAV_MESH_QUERY_DEBUG
	, m_batchCount(0)
	, m_queryDebug(*this)
#endif // NAV_MESH_QUERY_DEBUG
{
	InitializeRuntimeData();
	RegisterNavMeshCallbacks();
#ifdef NAV_MESH_QUERY_DEBUG
	DebugDrawQueryConfig();
#endif // NAV_MESH_QUERY_DEBUG
}

void CNavMeshQuery::OnNavMeshChanged(NavigationAgentTypeID navAgentId, NavigationMeshID navMeshId, TileID changedTileId)
{
	if (navAgentId != m_queryConfig.agentTypeId ||
		m_runtimeData.meshId != navMeshId ||
		m_status == INavMeshQuery::EQueryStatus::Done)
	{
		return;
	}

#ifdef NAV_MESH_QUERY_DEBUG
	const CTimeValue now = gEnv->pTimer->GetAsyncCurTime();
	bool regenerationAffectsQuery = false;
#endif //NAV_MESH_QUERY_DEBUG

	switch (m_queryConfig.actionOnNavMeshRegeneration)
	{
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::InvalidateAlways:
#ifdef NAV_MESH_QUERY_DEBUG
		regenerationAffectsQuery = true;
#endif //NAV_MESH_QUERY_DEBUG
		SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshRegeneration);
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::InvalidateIfProccesed:
		if (IsTileProcessed(changedTileId))
		{
#ifdef NAV_MESH_QUERY_DEBUG
			regenerationAffectsQuery = true;
#endif //NAV_MESH_QUERY_DEBUG
			SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshRegeneration);
		}
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::Ignore:
		if (m_runtimeData.stoppedAtTriangleIndexInIncompleteTile > 0)
		{
			const TileID currentTileId = m_runtimeData.pMesh->GetTileID(m_runtimeData.lastTileX, m_runtimeData.lastTileY, m_runtimeData.lastTileZ);
			if (changedTileId == currentTileId)
			{
				SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshRegeneration);
			}
		}
		break;
	default:
		CRY_ASSERT_MESSAGE(false, "New enum value has been added to MNM::INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange but it's not being handled in this switch statement");
	}

#ifdef NAV_MESH_QUERY_DEBUG
	if (regenerationAffectsQuery)
	{
		const INavMeshQueryDebug::SInvalidationData queryInvalidation(changedTileId, now, INavMeshQueryDebug::EInvalidationType::NavMeshRegeneration);
		m_queryDebug.AddInvalidationToHistory(queryInvalidation);
		DebugDrawQueryInvalidation(queryInvalidation);
	}
#endif //NAV_MESH_QUERY_DEBUG
}

void CNavMeshQuery::OnNavMeshAnnotationChanged(NavigationAgentTypeID navAgentId, NavigationMeshID navMeshId, TileID changedTileId)
{
	if (navAgentId != m_queryConfig.agentTypeId ||
		m_runtimeData.meshId != navMeshId ||
		m_status == INavMeshQuery::EQueryStatus::Done)
	{
		return;
	}

#ifdef NAV_MESH_QUERY_DEBUG
	const CTimeValue now = gEnv->pTimer->GetAsyncCurTime();
	bool changeAffectsQuery = false;
#endif //NAV_MESH_QUERY_DEBUG

	switch (m_queryConfig.actionOnNavMeshRegeneration)
	{
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::InvalidateAlways:
#ifdef NAV_MESH_QUERY_DEBUG
		changeAffectsQuery = true;
#endif //NAV_MESH_QUERY_DEBUG
		SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshAnnotationChanges);
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::InvalidateIfProccesed:
		if (IsTileProcessed(changedTileId))
		{
#ifdef NAV_MESH_QUERY_DEBUG
			changeAffectsQuery = true;
#endif //NAV_MESH_QUERY_DEBUG
			SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshAnnotationChanges);
		}
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange::Ignore:
		if (m_runtimeData.stoppedAtTriangleIndexInIncompleteTile > 0)
		{
			const TileID currentTileId = m_runtimeData.pMesh->GetTileID(m_runtimeData.lastTileX, m_runtimeData.lastTileY, m_runtimeData.lastTileZ);
			if (changedTileId == currentTileId)
			{
				SetStatus(INavMeshQuery::EQueryStatus::InvalidDueToNavMeshAnnotationChanges);
			}
		}
		break;
	default:
		CRY_ASSERT_MESSAGE(false, "New enum value has been added to MNM::INavMeshQuery::SNavMeshQueryConfig::EActionOnNavMeshChange but it's not being handled in this switch statement");
	}

#ifdef NAV_MESH_QUERY_DEBUG
	if (changeAffectsQuery)
	{
		const INavMeshQueryDebug::SInvalidationData queryInvalidation(changedTileId, now, INavMeshQueryDebug::EInvalidationType::NavMeshAnnotationChanged);
		m_queryDebug.AddInvalidationToHistory(queryInvalidation);
		DebugDrawQueryInvalidation(queryInvalidation);
	}
#endif //NAV_MESH_QUERY_DEBUG

}

void CNavMeshQuery::RegisterNavMeshCallbacks()
{
	gAIEnv.pNavigationSystem->AddMeshChangeCallback(m_queryConfig.agentTypeId, functor(*this, &CNavMeshQuery::OnNavMeshChanged));
	gAIEnv.pNavigationSystem->AddMeshAnnotationChangeCallback(m_queryConfig.agentTypeId, functor(*this, &CNavMeshQuery::OnNavMeshAnnotationChanged));
}

void CNavMeshQuery::DeregisterNavMeshCallbacks()
{
	gAIEnv.pNavigationSystem->RemoveMeshChangeCallback(m_queryConfig.agentTypeId, functor(*this, &CNavMeshQuery::OnNavMeshChanged));
	gAIEnv.pNavigationSystem->RemoveMeshAnnotationChangeCallback(m_queryConfig.agentTypeId, functor(*this, &CNavMeshQuery::OnNavMeshAnnotationChanged));
}

bool CNavMeshQuery::InitializeRuntimeData()
{
	const INavigationSystem* pNavigationSystem = gEnv->pAISystem->GetNavigationSystem();

	IF_UNLIKELY(!pNavigationSystem)
	{
		CRY_ASSERT_MESSAGE(!pNavigationSystem, "Couldn't retrieve Navigation System. NavMeshQuery cannot initialize RuntimeData and therefore it will not be executed");
		return false;
	}

	const NavigationMeshID meshID = pNavigationSystem->GetEnclosingMeshID(m_queryConfig.agentTypeId, m_queryConfig.position);
	const CNavMesh* pMesh = static_cast<const CNavMesh*>(pNavigationSystem->GetMNMNavMesh(meshID));

	if (!pMesh)
	{
		CRY_ASSERT_MESSAGE(!pMesh, "Couldn't retrieve agent enclosing mesh. NavMeshQuery cannot initialize RuntimeData and therefore it will not be executed");
		return false;
	}

	const CNavMesh::SGridParams meshParams = pMesh->GetGridParams();

	m_runtimeData.tileMinX = (std::max(vector3_t(m_queryConfig.aabb.min).x, real_t(0)) / real_t(meshParams.tileSize.x)).as_uint();
	m_runtimeData.tileMinY = (std::max(vector3_t(m_queryConfig.aabb.min).y, real_t(0)) / real_t(meshParams.tileSize.y)).as_uint();
	m_runtimeData.tileMinZ = (std::max(vector3_t(m_queryConfig.aabb.min).z, real_t(0)) / real_t(meshParams.tileSize.z)).as_uint();

	m_runtimeData.tileMaxX = (std::max(vector3_t(m_queryConfig.aabb.max).x, real_t(0)) / real_t(meshParams.tileSize.x)).as_uint();
	m_runtimeData.tileMaxY = (std::max(vector3_t(m_queryConfig.aabb.max).y, real_t(0)) / real_t(meshParams.tileSize.y)).as_uint();
	m_runtimeData.tileMaxZ = (std::max(vector3_t(m_queryConfig.aabb.max).z, real_t(0)) / real_t(meshParams.tileSize.z)).as_uint();

	m_runtimeData.lastTileX = m_runtimeData.tileMinX;
	m_runtimeData.lastTileY = m_runtimeData.tileMinY;
	m_runtimeData.lastTileZ = m_runtimeData.tileMinZ;

	m_runtimeData.meshId = meshID;
	m_runtimeData.currentTileCompleted = false;
	m_runtimeData.remainingTrianglesBudgetInBatch = 0;
	m_runtimeData.stoppedAtTriangleIndexInIncompleteTile = 0;

	return true;
}

bool CNavMeshQuery::IsTileProcessed(const TileID tileId) const
{
	TileIdSet::const_iterator it = m_processedTileIdSet.find(tileId);
	return it != m_processedTileIdSet.end();
}

bool CNavMeshQuery::AllTilesDone() const
{
	return m_runtimeData.lastTileX == m_runtimeData.tileMaxX &&
		m_runtimeData.lastTileY == m_runtimeData.tileMaxY &&
		m_runtimeData.lastTileZ == m_runtimeData.tileMaxZ &&
		m_runtimeData.currentTileCompleted;
}

void CNavMeshQuery::SetTileProcessed(const TileID tileId)
{
	m_processedTileIdSet.insert(tileId);
}

void CNavMeshQuery::SetStatus(const INavMeshQuery::EQueryStatus status)
{
	m_status = status;
}

INavMeshQuery::EQueryStatus CNavMeshQuery::PullNextTriangleBatchInternal(const size_t batchSize, TriangleIDArray& outTriangles)
{
	if (!SetupNavMeshInRuntimeData())
	{
		return INavMeshQuery::EQueryStatus::InvalidDueToCachedNavMeshIdNotFound;
	}

#ifdef NAV_MESH_QUERY_DEBUG
	const CTimeValue timeAtStart = gEnv->pTimer->GetAsyncCurTime();
#endif // NAV_MESH_QUERY_DEBUG
	size_t trianglesCount;
	ProcessTilesTriangles(batchSize, outTriangles, trianglesCount);
	outTriangles.resize(trianglesCount);

#ifdef NAV_MESH_QUERY_DEBUG
	const CTimeValue timeAtEnd = gEnv->pTimer->GetAsyncCurTime();
	const float elapsedTimeInMs = timeAtEnd.GetDifferenceInSeconds(timeAtStart) * 1000;
	const INavMeshQueryDebug::SBatchData queryBatch(m_batchCount++, batchSize, elapsedTimeInMs, m_runtimeData.pMesh, outTriangles);
	m_queryDebug.AddBatchToHistory(queryBatch);
	DebugDrawQueryBatch(queryBatch);
#endif // NAV_MESH_QUERY_DEBUG

	if (TryApplyNavMeshQueryProcessing(outTriangles) == INavMeshQueryProcessing::EResult::Stop ||
		outTriangles.size() < batchSize ||
		AllTilesDone())
	{
		return INavMeshQuery::EQueryStatus::Done;
	}

	return INavMeshQuery::EQueryStatus::Running;
}

bool CNavMeshQuery::SetupNavMeshInRuntimeData()
{
	m_runtimeData.pMesh = static_cast<const CNavMesh*>(gAIEnv.pNavigationSystem->GetMNMNavMesh(m_runtimeData.meshId));
	if (!m_runtimeData.pMesh)
	{
		return false;
	}

	return true;
}

void CNavMeshQuery::ProcessTilesTriangles(const size_t batchCount, TriangleIDArray& outTriangles, size_t& outTrianglesCountTotal)
{
	outTriangles.clear();
	outTriangles.resize(batchCount);
	outTrianglesCountTotal = 0;
	m_runtimeData.remainingTrianglesBudgetInBatch = batchCount;

	uint x = m_runtimeData.lastTileX;
	uint y = m_runtimeData.lastTileY;
	uint z = m_runtimeData.lastTileZ;

	for (; y <= m_runtimeData.tileMaxY; ++y)
	{
		x = x <= m_runtimeData.tileMaxX ? x : m_runtimeData.tileMinX;
		for (; x <= m_runtimeData.tileMaxX; ++x)
		{
			z = z <= m_runtimeData.tileMaxZ ? z : m_runtimeData.tileMinZ;
			for (; z <= m_runtimeData.tileMaxZ; ++z)
			{
				if (const TileID tileId = m_runtimeData.pMesh->GetTileID(x, y, z))
				{
					SetTileProcessed(tileId);

					size_t outTrianglesCount;
					size_t stoppedAtTriangleIndexInTile;
					m_runtimeData.currentTileCompleted = ProcessTileTriangles(tileId, batchCount, outTriangles, outTrianglesCount, stoppedAtTriangleIndexInTile);

					m_runtimeData.remainingTrianglesBudgetInBatch -= outTrianglesCount;
					outTrianglesCountTotal += outTrianglesCount;

					if (m_runtimeData.currentTileCompleted)
					{
						m_runtimeData.stoppedAtTriangleIndexInIncompleteTile = 0;
					}
					else
					{
						m_runtimeData.stoppedAtTriangleIndexInIncompleteTile = stoppedAtTriangleIndexInTile;
						m_runtimeData.lastTileX = x;
						m_runtimeData.lastTileY = y;
						m_runtimeData.lastTileZ = z;
						return;
					}
				}
			}
		}
	}

	m_runtimeData.lastTileX = x;
	m_runtimeData.lastTileY = y;
	m_runtimeData.lastTileZ = z;
}

bool CNavMeshQuery::ProcessTileTriangles(const TileID tileId, const size_t batchCount, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const
{
	const size_t processedTrianglesInBatch = batchCount - m_runtimeData.remainingTrianglesBudgetInBatch;
	const STile& tile = m_runtimeData.pMesh->GetTile(tileId);

	const CNavMesh::SGridParams gridParams = m_runtimeData.pMesh->GetGridParams();

	const vector3_t tileMin(0, 0, 0);
	const vector3_t tileMax(gridParams.tileSize.x, gridParams.tileSize.y, gridParams.tileSize.z);

	Tile::STileData tileData;
	m_runtimeData.pMesh->GetTileData(tileId, tileData);

	const aabb_t queryAabbTile(
		vector3_t::maximize(vector3_t(m_queryConfig.aabb.min) - tileData.tileOriginWorld, tileMin),
		vector3_t::minimize(vector3_t(m_queryConfig.aabb.max) - tileData.tileOriginWorld, tileMax)
	);

	if (m_queryConfig.pQueryFilter)
	{
		return QueryTileTriangles(tileId, tile, queryAabbTile, *m_queryConfig.pQueryFilter, processedTrianglesInBatch, outTriangles, outTrianglesCount, outStoppedAtTriangleIndexInTile);
	}
	else
	{
		return QueryTileTriangles(tileId, tile, queryAabbTile, DefaultQueryFilters::g_acceptAllTriangles, processedTrianglesInBatch, outTriangles, outTrianglesCount, outStoppedAtTriangleIndexInTile);
	}
}

INavMeshQueryProcessing::EResult CNavMeshQuery::TryApplyNavMeshQueryProcessing(const TriangleIDArray& triangleIDArray)
{
	if (!m_queryConfig.pQueryProcessing)
	{
		return INavMeshQueryProcessing::EResult::Continue;
	}

	return (*m_queryConfig.pQueryProcessing)(triangleIDArray);
}

template<typename TFilter>
bool CNavMeshQuery::QueryTileTriangles(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const
{
	if (tile.GetBVNodesCount() > 0)
	{
		return QueryTileTrianglesBV(tileId, tile, queryAabbTile, filter, processedTrianglesInBatch, outTriangles, outTrianglesCount, outStoppedAtTriangleIndexInTile);
	}
	else
	{
		return QueryTileTrianglesLinear(tileId, tile, queryAabbTile, filter, processedTrianglesInBatch, outTriangles, outTrianglesCount, outStoppedAtTriangleIndexInTile);
	}
}

template<typename TFilter>
bool CNavMeshQuery::QueryTileTrianglesBV(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const
{
	size_t trianglesCount = 0;
	size_t nodeID = m_runtimeData.stoppedAtTriangleIndexInIncompleteTile;
	bool currentTileCompleted = true;
	outStoppedAtTriangleIndexInTile = 0;

	while (nodeID < tile.GetBVNodesCount())
	{
		if (trianglesCount == m_runtimeData.remainingTrianglesBudgetInBatch)
		{
			currentTileCompleted = false;
			outStoppedAtTriangleIndexInTile = nodeID;
			break;
		}

		const Tile::SBVNode& node = tile.GetBVNodes()[nodeID];
		const uint16 triangleIdx = node.offset;
		const TriangleID triangleId = ComputeTriangleID(tileId, triangleIdx);

		if (!TrianglesBBoxIntersectionBV(queryAabbTile, node.aabb, tileId, triangleId))
		{
			nodeID += node.leaf ? 1 : node.offset;
		}
		else
		{
			++nodeID;

			if (node.leaf)
			{
				const Tile::STriangle& triangle = tile.GetTriangles()[triangleIdx];

				if (filter.PassFilter(triangle))
				{
					outTriangles[processedTrianglesInBatch + trianglesCount] = triangleId;

					++trianglesCount;
				}
			}
		}
	}

	outTrianglesCount = trianglesCount;
	return currentTileCompleted;
}

template<typename TFilter>
bool CNavMeshQuery::QueryTileTrianglesLinear(const TileID tileId, const STile& tile, const aabb_t& queryAabbTile, TFilter& filter, const size_t processedTrianglesInBatch, TriangleIDArray& outTriangles, size_t& outTrianglesCount, size_t& outStoppedAtTriangleIndexInTile) const
{
	size_t trianglesCount = 0;
	bool currentTileCompleted = true;
	outStoppedAtTriangleIndexInTile = 0;

	for (size_t i = m_runtimeData.stoppedAtTriangleIndexInIncompleteTile; i < tile.GetTrianglesCount(); ++i)
	{
		if (trianglesCount == m_runtimeData.remainingTrianglesBudgetInBatch)
		{
			currentTileCompleted = false;
			outStoppedAtTriangleIndexInTile = i;
			break;
		}

		const Tile::STriangle& triangle = tile.GetTriangles()[i];

		const Tile::Vertex& v0 = tile.GetVertices()[triangle.vertex[0]];
		const Tile::Vertex& v1 = tile.GetVertices()[triangle.vertex[1]];
		const Tile::Vertex& v2 = tile.GetVertices()[triangle.vertex[2]];

		if (TrianglesBBoxIntersectionLinear(queryAabbTile, v0, v1, v2))
		{
			if (filter.PassFilter(triangle))
			{
				const TriangleID triangleId = ComputeTriangleID(tileId, static_cast<uint16>(i));
				outTriangles[processedTrianglesInBatch + trianglesCount] = triangleId;

				++trianglesCount;					
			}
		}
	}

	outTrianglesCount = trianglesCount;
	return currentTileCompleted;
}

bool CNavMeshQuery::TrianglesBBoxIntersectionBV(const aabb_t& queryAabbTile, const aabb_t& nodeAabb_, const TileID tileId, const TriangleID triangleId) const
{
	const uint16 triangleIdx = ComputeTriangleIndex(triangleId);
	Tile::STileData tileData;
	m_runtimeData.pMesh->GetTileData(tileId, tileData);

	const vector3_t v0 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 0).GetVec3();
	const vector3_t v1 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 1).GetVec3();
	const vector3_t v2 = Tile::Util::GetVertexPosWorldFixed(tileData, triangleIdx, 2).GetVec3();

	return TrianglesBBoxIntersectionHelper(queryAabbTile, nodeAabb_, v0, v1, v2);
}

bool CNavMeshQuery::TrianglesBBoxIntersectionLinear(const aabb_t& queryAabbTile, const vector3_t& v0, const vector3_t& v1, const vector3_t& v2) const
{
	const aabb_t triangleAabb(vector3_t::minimize(v0, v1, v2), vector3_t::maximize(v0, v1, v2));
	return TrianglesBBoxIntersectionHelper(queryAabbTile, triangleAabb, v0, v1, v2);
}

bool CNavMeshQuery::TrianglesBBoxIntersectionHelper(const aabb_t& queryAabbTile, const aabb_t& triangleAabb, const vector3_t& v0, const vector3_t& v1, const vector3_t& v2) const
{
	bool result = false;
	switch (m_queryConfig.overlappingMode)
	{
	case INavMeshQuery::SNavMeshQueryConfig::EOverlappingMode::BoundingBox_Partial:
		result = queryAabbTile.overlaps(triangleAabb);
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EOverlappingMode::BoundingBox_Full:
		result = queryAabbTile.contains(triangleAabb);
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EOverlappingMode::Triangle_Partial:
		result = queryAabbTile.overlaps(v0, v1, v2);
		break;
	case INavMeshQuery::SNavMeshQueryConfig::EOverlappingMode::Triangle_Full:
		result = queryAabbTile.contains(v0) && queryAabbTile.contains(v1) && queryAabbTile.contains(v2);
		break;
	default:
		CRY_ASSERT_MESSAGE(false, "New enum value has been added to MNM::INavMeshQuery::SNavMeshQueryConfig::IntersectionMode but it's not being handled in this switch statement");
	}
	return result;
}

#ifdef NAV_MESH_QUERY_DEBUG
void CNavMeshQuery::DebugDrawQueryConfig() const
{
	if (!gAIEnv.CVars.DebugDrawNavigationQueriesUDR)
	{
		return;
	}

	const ColorF queryAabbColor = Col_Yellow;
	const ColorF agentPositionColor = Col_Green;
	const ColorF textColor = Col_White;
	const float  textSize = 1.5f;

	Cry::UDR::CScope_FixedString navMeshQueriesScopes("NavMeshQueries");
	{
		Cry::UDR::CScope_FixedString callerScope(GetQueryConfig().szCallerName);
		{
			stack_string queryIdText;
			queryIdText.Format("Query %zu", m_queryId);
			Cry::UDR::CScope_FixedString queryScope(queryIdText.c_str());
			{
				Cry::UDR::CScope_FixedString configScope("Config");
				{
					configScope->AddAABB(m_queryConfig.aabb, queryAabbColor);
					configScope->AddSphere(m_queryConfig.position, 0.25f, agentPositionColor);

					const Vec3 spacing(0.0f, 0.0f, 0.25f);
					Vec3 textPosition(m_queryConfig.position);

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Query Processing: %s", m_queryConfig.pQueryProcessing ? " Yes" : "No");

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Query Filter: %s", m_queryConfig.pQueryFilter ? " Yes" : "No");

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Action On NavMesh Annotation Change: %s", Serialization::getEnumLabel(m_queryConfig.actionOnNavMeshAnnotationChange));

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Action On NavMesh Change: %s", Serialization::getEnumLabel(m_queryConfig.actionOnNavMeshRegeneration));

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Intersection Mode: %s", Serialization::getEnumLabel(m_queryConfig.overlappingMode));

					textPosition += spacing;
					configScope->AddText(textPosition, textSize, textColor, "Agent Type: %s", GetAISystem()->GetNavigationSystem()->GetAgentTypeName(m_queryConfig.agentTypeId));
				}
			}
		}
	}
}

void CNavMeshQuery::DebugDrawQueryBatch(const INavMeshQueryDebug::SBatchData& queryBatch) const
{
	if (!gAIEnv.CVars.DebugDrawNavigationQueriesUDR)
	{
		return;
	}

	const ColorF trianglesColor = Col_Azure;

	Cry::UDR::CScope_FixedString navMeshQueriesScopes("NavMeshQueries");
	{
		Cry::UDR::CScope_FixedString callerScope(m_queryConfig.szCallerName);
		{
			stack_string queryIdText;
			queryIdText.Format("Query %zu", m_queryId);
			Cry::UDR::CScope_FixedString queryScope(queryIdText.c_str());
			{
				Cry::UDR::CScope_FixedString batchesScope("Batches");
				{
					Cry::UDR::CScope_UniqueStringAutoIncrement batchScope("Batch ");
					{
						for (size_t i = 0; i < queryBatch.triangleDataArray.size(); ++i)
						{
							const INavMeshQueryDebug::SBatchData::STriangleData triangleData = queryBatch.triangleDataArray[i];
							DebugDrawTriangleUDR(triangleData.triangleID, triangleData.v0, triangleData.v1, triangleData.v2, batchScope, trianglesColor);
						}
					}
				}
			}
		}
	}
}

void CNavMeshQuery::DebugDrawQueryInvalidation(const INavMeshQueryDebug::SInvalidationData& queryInvalidation) const
{
	if (!gAIEnv.CVars.DebugDrawNavigationQueriesUDR)
	{
		return;
	}

	const ColorF invalidTrianglesColor = Col_Red;

	Cry::UDR::CScope_FixedString navMeshQueriesScopes("NavMeshQueries");
	{
		Cry::UDR::CScope_FixedString callerScope(m_queryConfig.szCallerName);
		{
			stack_string queryIdText;
			queryIdText.Format("Query %zu", m_queryId);
			Cry::UDR::CScope_FixedString queryScope(queryIdText.c_str());
			{
				stack_string scopeName;
				switch (queryInvalidation.invalidationType)
				{
				case INavMeshQueryDebug::EInvalidationType::NavMeshRegeneration:
					scopeName = "NavMesh Regeneration";
					break;
				case INavMeshQueryDebug::EInvalidationType::NavMeshAnnotationChanged:
					scopeName = "Annotation Changes";
					break;
				default:
					CRY_ASSERT_MESSAGE(false, "New enum value has been added to MNM::NavMeshQueryDebug::InvalidationType: but it's not being handled in this switch statement");
				}

				Cry::UDR::CScope_FixedString invalidationsScope(scopeName.c_str());
				{
					const CNavMesh* pMesh = GetQueryEnclosingMesh(m_queryConfig);
					if (!pMesh)
					{
						return;
					}

					Tile::STileData tileData;
					pMesh->GetTileData(queryInvalidation.tileID, tileData);

					for (size_t i = 0; i < tileData.trianglesCount; ++i)
					{
						const uint16 triangleIdx = static_cast<uint16>(i);
						const TriangleID triangleId = ComputeTriangleID(queryInvalidation.tileID, triangleIdx);

						vector3_t v0;
						vector3_t v1;
						vector3_t v2;

						if (pMesh->GetVertices(triangleId, v0, v1, v2))
						{
							DebugDrawTriangleUDR(triangleId, v0, v1, v2, invalidationsScope, invalidTrianglesColor);
						}
					}
				}
			}
		}
	}
}

const CNavMesh* CNavMeshQuery::GetQueryEnclosingMesh(const INavMeshQuery::SNavMeshQueryConfig& queryConfig) const
{
	const NavigationMeshID meshID = GetAISystem()->GetNavigationSystem()->GetEnclosingMeshID(queryConfig.agentTypeId, queryConfig.position);
	const CNavMesh* pMesh = static_cast<const CNavMesh*>(GetAISystem()->GetNavigationSystem()->GetMNMNavMesh(meshID));
	return pMesh;
}

void CNavMeshQuery::DebugDrawTriangleUDR(const TriangleID triangleID, const vector3_t v0_, const vector3_t v1_, const vector3_t v2_, Cry::UDR::CScopeBase& scope, const ColorF& triangleFillColor) const
{
	const Vec3 v0 = v0_.GetVec3();
	const Vec3 v1 = v1_.GetVec3();
	const Vec3 v2 = v2_.GetVec3();

	const ColorF triangleEdgeColor = Col_White;
	scope->AddLine(v0, v1, triangleEdgeColor);
	scope->AddLine(v1, v2, triangleEdgeColor);
	scope->AddLine(v2, v0, triangleEdgeColor);
	scope->AddTriangle(v0, v1, v2, triangleFillColor);

	const ColorF textColor = Col_White;
	const float  textSize = 1.25f;
	scope->AddText((v0 + v1 + v2) / 3.f, textSize, textColor, "Id: %u", triangleID);
}
#endif // NAV_MESH_QUERY_DEBUG
} // namespace MNM