// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQueryFilter.h>

//! Default final implementation of INavMeshQueryFilter used in AI System.
//! Flag masks with combination of triangle flags are used for filtering passable triangles.
struct SNavMeshQueryFilterDefault final : public SNavMeshQueryFilterBase<SNavMeshQueryFilterDefault>
{
	virtual INavMeshQueryFilter* Clone() const override
	{
		return new SNavMeshQueryFilterDefault(*this);
	}

	virtual void Release() const override
	{
		delete this;
	}
};

//! Default final implementation of INavMeshQueryFilter with costs used in AI System.
//! Flag masks with combination of triangle flags are used for filtering passable triangles
//! and area cost multipliers for each triangle area type are used for computing the costs in queries such path-finding, etc.
struct SNavMeshQueryFilterDefaultWithCosts final : public SNavMeshQueryFilterBase<SNavMeshQueryFilterDefaultWithCosts>
{
	enum { MaxAreasCount = MNM::AreaAnnotation::MaxAreasCount() };
	typedef SNavMeshQueryFilterBase<SNavMeshQueryFilterDefaultWithCosts> TBaseFilter;

	SNavMeshQueryFilterDefaultWithCosts()
	{
		for (size_t i = 0; i < MaxAreasCount; ++i)
		{
			costs[i] = 1.0f;
		}
	}

	SNavMeshQueryFilterDefaultWithCosts(const SNavMeshQueryFilterDefaultWithCosts& other)
		: TBaseFilter(other)
	{
		memcpy(costs, other.costs, sizeof(costs[0]) * MaxAreasCount);
	}

	bool operator==(const SNavMeshQueryFilterDefaultWithCosts& other) const
	{
		if (!TBaseFilter::operator==(other))
			return false;

		for (size_t i = 0; i < MaxAreasCount; ++i)
		{
			if (costs[i] != other.costs[i])
				return false;
		}
		return true;
	}

	virtual INavMeshQueryFilter* Clone() const override
	{
		return new SNavMeshQueryFilterDefaultWithCosts(*this);
	}

	virtual void Release() const override
	{
		delete this;
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

	float costs[MaxAreasCount];
};

//! SAcceptAllQueryTrianglesFilter accepts all triangles
struct SAcceptAllQueryTrianglesFilter final : public SNavMeshQueryFilterBase<SAcceptAllQueryTrianglesFilter>
{
	typedef SNavMeshQueryFilterBase<SAcceptAllQueryTrianglesFilter> TBaseFilter;

	SAcceptAllQueryTrianglesFilter()
	{
		// Empty
	}

	SAcceptAllQueryTrianglesFilter(const SAcceptAllQueryTrianglesFilter& other)
		: TBaseFilter(other)
	{
		// Empty
	}

	virtual INavMeshQueryFilter* Clone() const override
	{
		return new SAcceptAllQueryTrianglesFilter(*this);
	}

	virtual void Release() const override
	{
		delete this;
	}

	bool operator==(const SAcceptAllQueryTrianglesFilter& other) const
	{
		return TBaseFilter::operator==(other);
	}

	virtual bool PassFilter(const MNM::AreaAnnotation annotation) const override
	{
		return true;
	}

	virtual bool PassFilter(const MNM::Tile::STriangle& triangle) const override
	{
		return true;
	}

	virtual float GetCostMultiplier(const MNM::Tile::STriangle& triangle) const override
	{
		return 1.0f;
	}

	virtual MNM::real_t GetCost(const MNM::vector3_t& fromPos, const MNM::vector3_t& toPos,
		const MNM::Tile::STriangle& currentTriangle, const MNM::Tile::STriangle& nextTriangle,
		const MNM::TriangleID currentTriangleId, const MNM::TriangleID nextTriangleId) const override
	{
		return (fromPos - toPos).lenNoOverflow();
	}
};