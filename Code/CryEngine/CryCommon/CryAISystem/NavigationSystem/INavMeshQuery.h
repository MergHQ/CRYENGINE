// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/MNMTile.h>
#include <CryAISystem/NavigationSystem/NavigationIdTypes.h>

#ifndef _RELEASE
#define NAV_MESH_QUERY_DEBUG
#endif // _RELEASE

namespace MNM
{
	typedef DynArray<TriangleID> TriangleIDArray;
namespace Tile
{
	struct STriangle;
}
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

//! INavMeshQueryFilter is a structure used in navigation queries (finding path, raycast, query triangles...)
//! for filtering passable triangles and computing costs of traversing triangles
//! 
//! Asynchronous jobs inside Navigation System can create a copy of the filter,
//! therefore filter implementations need to provide overridden Clone() and Release() functions. 
struct INavMeshQueryFilter
{
	virtual ~INavMeshQueryFilter() {}
	
	//! Clone method that should return new deep copy of the object
	virtual INavMeshQueryFilter* Clone() const = 0;
	
	//! Release method used for destroying the object
	virtual void Release() const = 0;

	//! Returns false if the annotation isn't passable
	virtual bool PassFilter(const MNM::AreaAnnotation annotation) const = 0;

	//! Returns false if the triangle isn't passable
	virtual bool PassFilter(const MNM::Tile::STriangle& triangle) const = 0;

	//! Returns cost multiplier for traversing the triangle
	virtual float GetCostMultiplier(const MNM::Tile::STriangle& triangle) const = 0;

	//! Currently not used but planned be used in the future instead of GetCostMultiplier
	virtual MNM::real_t GetCost(const MNM::vector3_t& fromPos, const MNM::vector3_t& toPos,
		const MNM::Tile::STriangle& currentTriangle, const MNM::Tile::STriangle& nextTriangle,
		const MNM::TriangleID currentTriangleId, const MNM::TriangleID nextTriangleId) const = 0;
};

DECLARE_SHARED_POINTERS(INavMeshQueryFilter);

//! Default base implementation of INavMeshQueryFilter
//! Flag masks with combination with triangle flags are used for filtering passable triangles
//! and area costs multipliers for each triangle area type are used for computing the costs
struct SNavMeshQueryFilterDefaultBase : public INavMeshQueryFilter
{
	typedef MNM::AreaAnnotation::value_type ValueType;
	enum { MaxAreasCount = MNM::AreaAnnotation::MaxAreasCount() };

	SNavMeshQueryFilterDefaultBase()
		: includeFlags(-1)
		, excludeFlags(0)
	{
		for (size_t i = 0; i < MaxAreasCount; ++i)
		{
			costs[i] = 1.0f;
		}
	}
	SNavMeshQueryFilterDefaultBase(const SNavMeshQueryFilterDefaultBase& other)
		: includeFlags(other.includeFlags)
		, excludeFlags(other.excludeFlags)
	{
		memcpy(costs, other.costs, sizeof(costs[0]) * MaxAreasCount);
	}

	virtual bool PassFilter(const MNM::AreaAnnotation annotation) const override
	{
		return (annotation.GetFlags() & includeFlags) && !(annotation.GetFlags() & excludeFlags);
	}

	virtual bool PassFilter(const MNM::Tile::STriangle& triangle) const override
	{
		return (triangle.areaAnnotation.GetFlags() & includeFlags) && !(triangle.areaAnnotation.GetFlags() & excludeFlags);
	}

	virtual float GetCostMultiplier(const MNM::Tile::STriangle& triangle) const override
	{
		CRY_ASSERT(triangle.areaAnnotation.GetType() < MaxAreasCount);
		return costs[triangle.areaAnnotation.GetType()];
	}

	virtual MNM::real_t GetCost(const MNM::vector3_t& fromPos, const MNM::vector3_t& toPos,
		const MNM::Tile::STriangle& currentTriangle, const MNM::Tile::STriangle& nextTriangle,
		const MNM::TriangleID currentTriangleId, const MNM::TriangleID nextTriangleId) const override
	{
		CRY_ASSERT(currentTriangle.areaAnnotation.GetType() < MaxAreasCount);
		return (fromPos - toPos).lenNoOverflow() * (MNM::real_t)costs[currentTriangle.areaAnnotation.GetType()];
	}

	void SetAreaCost(const ValueType areaIdx, const float cost)
	{
		CRY_ASSERT(areaIdx >= 0 && areaIdx < MaxAreasCount);
		costs[areaIdx] = cost;
	}
	float GetAreaCost(const ValueType areaIdx) const
	{
		CRY_ASSERT(areaIdx >= 0 && areaIdx < MaxAreasCount);
		return costs[areaIdx];
	}

	void SetIncludeFlags(const ValueType flags) { includeFlags = flags; }
	ValueType GetIncludeFlags() const { return includeFlags; }

	void SetExcludeFlags(const ValueType flags) { excludeFlags = flags; }
	ValueType GetExcludeFlags() const { return excludeFlags; }

	ValueType includeFlags;
	ValueType excludeFlags;

	float costs[MaxAreasCount];
};

//! Default final implementation of INavMeshQueryFilter used in AI System
//! Flag masks with combination with triangle flags are used for filtering passable triangles
//! and area costs multipliers for each triangle area type are used for computing the costs
struct SNavMeshQueryFilterDefault final : public SNavMeshQueryFilterDefaultBase
{
	virtual INavMeshQueryFilter* Clone() const override
	{
		return new SNavMeshQueryFilterDefault(*this);
	}

	virtual void Release() const override
	{
		delete this;
	}

	bool operator==(const SNavMeshQueryFilterDefault& other) const
	{
		return includeFlags == other.includeFlags && excludeFlags == other.excludeFlags
			&& !memcmp(costs, other.costs, sizeof(costs[0]) * MaxAreasCount);
	}
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