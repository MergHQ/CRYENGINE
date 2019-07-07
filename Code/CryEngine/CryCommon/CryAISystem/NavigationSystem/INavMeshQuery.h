// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/MNMTile.h>
#include <CryAISystem/NavigationSystem/NavigationIdTypes.h>
#include <CryAISystem/NavigationSystem/INavMeshQueryFilter.h>

#ifndef _RELEASE
#define NAV_MESH_QUERY_DEBUG
#endif // _RELEASE

struct INavMeshQueryProcessing;

namespace MNM
{
	typedef size_t NavMeshQueryId;
	typedef DynArray<TriangleID> TriangleIDArray;

#ifdef NAV_MESH_QUERY_DEBUG
	struct INavMeshQueryDebug;
#endif // NAV_MESH_QUERY_DEBUG

	//! INavMeshQuery is used for querying the NavMesh using a INavMeshQueryProcessing
	//! that takes the NavMesh triangles resulting from the query 
	//! and applies an extra (processing) operation
	struct INavMeshQuery
	{
		//! SNavMeshQueryConfig is used to hold the configuration parameters of a query
		//! The parameters specify how the query should be performed.
		//! They are set only once before executing the query
		struct SNavMeshQueryConfig
		{
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
#ifdef NAV_MESH_QUERY_DEBUG
			void DebugDraw() const;
#endif // NAV_MESH_QUERY_DEBUG

		protected:
			SNavMeshQueryConfig(const NavigationMeshID meshId_,
				const char* szCallerName_,
				const aabb_t& aabb_,
				const EOverlappingMode overlappingMode_ = EOverlappingMode::BoundingBox_Partial,
				const INavMeshQueryFilter* pQueryFilter_ = nullptr)
				: meshId(meshId_)
				, szCallerName(szCallerName_)
				, aabb(aabb_)
				, overlappingMode(overlappingMode_)
				, pQueryFilter(pQueryFilter_)
			{
			}

		public:
			const NavigationMeshID         meshId;
			const char*                    szCallerName;
			const aabb_t                   aabb;
			const EOverlappingMode         overlappingMode;
			const INavMeshQueryFilter*     pQueryFilter;
		};

		//! SNavMeshQueryConfigInstant is a specialized SNavMeshQueryConfig used to perform CNavMeshQueryInstant that will run only once in one frame.
		struct SNavMeshQueryConfigInstant final : public SNavMeshQueryConfig
		{
			SNavMeshQueryConfigInstant(const NavigationMeshID meshId,
				const char* szCallerName_,
				const aabb_t& aabb_,
				const EOverlappingMode overlappingMode_ = EOverlappingMode::BoundingBox_Partial,
				const INavMeshQueryFilter* pQueryFilter_ = nullptr)
				: SNavMeshQueryConfig(meshId, szCallerName_, aabb_, overlappingMode_, pQueryFilter_)
			{
				// Empty
			}

		};

		//! SNavMeshQueryConfigBatch is a specialized SNavMeshQueryConfig used to perform CNavMeshQueryBatch that may run multiple times.
		//! Since the NavMeshConfig may live during multiple frames, it holds a copy of the passed NavMeshQueryFilter to have control over
		//! its lifetime. When the config is destroyed, the copy of the filter is destroyed as well.
		struct SNavMeshQueryConfigBatch final : public SNavMeshQueryConfig
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

			SNavMeshQueryConfigBatch(const NavigationMeshID meshId,
				const char* szCallerName_,
				const aabb_t& aabb_,
				const EOverlappingMode overlappingMode_ = EOverlappingMode::BoundingBox_Partial,
				const INavMeshQueryFilter* pQueryFilter_ = nullptr,
				const size_t processingTrianglesMaxSize_ = 1024,
				const EActionOnNavMeshChange actionOnNavMeshRegeneration_ = EActionOnNavMeshChange::Ignore,
				const EActionOnNavMeshChange actionOnNavMeshAnnotationChange_ = EActionOnNavMeshChange::Ignore)
				: SNavMeshQueryConfig(meshId, szCallerName_, aabb_, overlappingMode_, pQueryFilter_ ? pQueryFilter_->Clone() : nullptr)
				, processingTrianglesMaxSize(processingTrianglesMaxSize_)
				, actionOnNavMeshRegeneration(actionOnNavMeshRegeneration_)
				, actionOnNavMeshAnnotationChange(actionOnNavMeshAnnotationChange_)
			{
				// Empty
			}

			SNavMeshQueryConfigBatch(const SNavMeshQueryConfigBatch& other)
				: SNavMeshQueryConfig(other.meshId, other.szCallerName, other.aabb, other.overlappingMode, other.pQueryFilter ? other.pQueryFilter->Clone() : nullptr)
				, processingTrianglesMaxSize(other.processingTrianglesMaxSize)
				, actionOnNavMeshRegeneration(other.actionOnNavMeshRegeneration)
				, actionOnNavMeshAnnotationChange(other.actionOnNavMeshAnnotationChange)
			{
			}

			~SNavMeshQueryConfigBatch()
			{
				if (pQueryFilter)
					pQueryFilter->Release();
			}

#ifdef NAV_MESH_QUERY_DEBUG
			void DebugDraw() const;
#endif // NAV_MESH_QUERY_DEBUG

			const size_t                   processingTrianglesMaxSize;
			const EActionOnNavMeshChange   actionOnNavMeshRegeneration;
			const EActionOnNavMeshChange   actionOnNavMeshAnnotationChange;
		};

		//! Describes the execution status of the INavMeshQuery
		enum class EQueryStatus
		{
			Uninitialized,
			Running,
			Done_QueryProcessingStopped,
			Done_Complete,
			Invalid_InitializationFailed,
			Invalid_NavMeshRegenerated,
			Invalid_NavMeshAnnotationChanged,
			Invalid_CachedNavMeshIdNotFound,

			Count = 8
		};

		virtual NavMeshQueryId                  GetId() const = 0;
		virtual const SNavMeshQueryConfig&      GetQueryConfig() const = 0;
		virtual EQueryStatus                    GetStatus() const = 0;
		virtual bool                            IsValid() const = 0;
		virtual bool                            IsDone() const = 0;
		virtual bool                            IsRunning() const = 0;

#ifdef NAV_MESH_QUERY_DEBUG
		virtual const INavMeshQueryDebug&       GetQueryDebug() const = 0;
		virtual bool operator< (const INavMeshQuery & other) const = 0;
#endif // NAV_MESH_QUERY_DEBUG

		//! Runs the query with the specified configuration in the constructor. Resulting triangles are passed to the queryProcessing in batches of given size (by default 32)
		//! /param queryProcessing Applies a processing operation on the resulting triangles from the query
		//! /param processingBatchSize specifies the maximum size of triangles that can be processed in a single batch. By default is 32 (query processing will be executed every 32 triangles).
		//! /return Status of the query after the Run operation has been completed
		virtual INavMeshQuery::EQueryStatus     Run(INavMeshQueryProcessing& queryProcessing, const size_t processingBatchSize = 32) = 0;
	protected:
		~INavMeshQuery() = default;
	};

	DECLARE_SHARED_POINTERS(INavMeshQuery)

	typedef INavMeshQuery::SNavMeshQueryConfig::EOverlappingMode ENavMeshQueryOverlappingMode;
}