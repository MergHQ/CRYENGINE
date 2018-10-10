// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/MNMTile.h>
#include <CryAISystem/NavigationSystem/NavigationIdTypes.h>
#include <CryAISystem/NavigationSystem/INavMeshQueryFilter.h>

#ifndef _RELEASE
#define NAV_MESH_QUERY_DEBUG
#endif // _RELEASE

namespace MNM
{
	typedef DynArray<TriangleID> TriangleIDArray;
}

//! INavMeshQueryProcessing is a structure used for processing triangles in NavMesh triangle queries. See INavMesh::QueryTrianglesWithProcessing
struct INavMeshQueryProcessing
{
	enum class EResult
	{
		Continue, // Continue processing with next set of triangles
		Stop,     // Processing is done and should be stopped
	};
	virtual EResult operator() (const MNM::TriangleIDArray& triangleIDArray) = 0;
};

namespace MNM
{

typedef size_t NavMeshQueryId;

#ifdef NAV_MESH_QUERY_DEBUG
struct INavMeshQueryDebug;
#endif // NAV_MESH_QUERY_DEBUG

struct INavMeshQuery
{
public:
	struct SNavMeshQueryConfig
	{
		//! Specify how (incomplete) NavMeshQueries should react to NavMesh changes
		enum class EActionOnNavMeshChange
		{
			//! Query ignores the NavMesh change and keeps processing triangles unless NavMesh change happens in the same tile the query is processing
			Ignore,

			//! Query immediately gets invalidated and stops processing when the NavMesh changes
			InvalidateAlways,

			//! InvalidateIfProccesed: Query only gets invalidated and stops processing when the invalidated NavMesh Tile has already been processed by the Query. If this does not apply, query keeps processing triangles.
			InvalidateIfProccesed
		};

		//! Specify how overlapping between a NavMesh triangle and a QueryAABB should be calculated
		enum class EOverlappingMode
		{
			//! Returns true if there's any overlap between TriangleAABB and QueryAABB.
			BoundingBox_Partial,

			//! Returns true if TriangleAABB is inside QueryAABB
			BoundingBox_Full,

			//! Returns true if the Triangle intersects the QueryAABB
			Triangle_Partial,

			//! Returns true if all three vertices of the Triangle are inside the QueryAABB
			Triangle_Full
		};

		//! Describes the query configuration
		//! /param pQueryFilter_ is only used to perform a deep copy
		//! /param pQueryProcessing_ should persist while the query is running
		SNavMeshQueryConfig(const NavigationAgentTypeID agentTypeId_, 
			const char* szCallerName_, 
			const AABB& aabb_, 
			const Vec3& position_, 
			const EActionOnNavMeshChange actionOnNavMeshRegeneration_ = EActionOnNavMeshChange::Ignore,
			const EActionOnNavMeshChange actionOnNavMeshAnnotationChange_ = EActionOnNavMeshChange::Ignore,
			const EOverlappingMode overlappingMode_ = EOverlappingMode::BoundingBox_Partial,
			const INavMeshQueryFilter* pQueryFilter_ = nullptr,
			INavMeshQueryProcessing* pQueryProcessing_ = nullptr)
			: agentTypeId(agentTypeId_)
			, szCallerName(szCallerName_)
			, aabb(aabb_)
			, position(position_)
			, actionOnNavMeshRegeneration(actionOnNavMeshRegeneration_)
			, actionOnNavMeshAnnotationChange(actionOnNavMeshAnnotationChange_)
			, overlappingMode(overlappingMode_)
			, pQueryFilter(nullptr)
			, pQueryProcessing(pQueryProcessing_)
		{
			if (pQueryFilter_)
			{
				pQueryFilter = pQueryFilter_->Clone();
			}
		}

		~SNavMeshQueryConfig()
		{
			if (pQueryFilter)
			{
				pQueryFilter->Release();
			}
		}

		const NavigationAgentTypeID    agentTypeId;
		const char*                    szCallerName;
		const AABB                     aabb;
		const Vec3                     position;
		const EActionOnNavMeshChange   actionOnNavMeshRegeneration;
		const EActionOnNavMeshChange   actionOnNavMeshAnnotationChange;
		const EOverlappingMode         overlappingMode;
		const INavMeshQueryFilter*     pQueryFilter;
		INavMeshQueryProcessing*       pQueryProcessing;
	};

	//! Describes the execution status of the NavMeshQuery
	enum class EQueryStatus
	{
		Running,
		Done,
		InvalidDueToNavMeshRegeneration,
		InvalidDueToNavMeshAnnotationChanges,
		InvalidDueToCachedNavMeshIdNotFound
	};

	virtual NavMeshQueryId                  GetId() const = 0;
	virtual const SNavMeshQueryConfig&      GetQueryConfig() const = 0;
	virtual EQueryStatus                    GetStatus() const = 0;
	virtual bool                            HasInvalidStatus() const = 0;

	virtual EQueryStatus                    PullTrianglesBatch(const size_t batchCount, TriangleIDArray& outTriangles) = 0;

#ifdef NAV_MESH_QUERY_DEBUG
	virtual const INavMeshQueryDebug&       GetQueryDebug() const = 0;
#endif // NAV_MESH_QUERY_DEBUG
protected:
	~INavMeshQuery() {}
};

DECLARE_SHARED_POINTERS(INavMeshQuery)
}