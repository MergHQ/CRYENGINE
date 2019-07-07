// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQueryProcessing.h>
#include "NavMesh.h"
#include "Navigation/NavigationSystem/NavigationSystem.h"

namespace MNM
{	
	//! CNavMeshQueryProcessingBase is the base class for any NavMeshQueryProcessing
	//! It implements the common methods
	class CNavMeshQueryProcessingBase : public INavMeshQueryProcessing
	{
	public:
		virtual void         Clear() override { m_triangleIdArray.clear(); }

		virtual size_t       Size() const override { return m_triangleIdArray.size(); }

		virtual void         AddTriangle(const TriangleID triangleId) override { m_triangleIdArray.push_back(triangleId); }

		virtual void         AddTriangleArray(const DynArray<TriangleID>& triangleIdArray) override { m_triangleIdArray.push_back(triangleIdArray); }

	protected:
		CNavMeshQueryProcessingBase()
		{
			// Empty
		}

		DynArray<TriangleID> m_triangleIdArray;
	};

	//! CDefaultProcessing does not apply any processing to the provided triangles.
	//! It simply provides access to the stored triangles id array.
	class CDefaultProcessing final : public CNavMeshQueryProcessingBase
	{
	public:
		CDefaultProcessing()
		{
			// Empty
		}

		const DynArray<TriangleID>&              GetTriangleIdArray() const { return m_triangleIdArray; }

		virtual INavMeshQueryProcessing::EResult Process() override;
	};


	//! CNavMeshQueryProcessing is the base class for NavMeshQueryProcessing that involves any NavMesh operation (through a NavMesh pointer)
	//! Since storing the NavMesh pointer could be dangerous, this class offers a function to Update the NavMesh pointer from the stored mesh id
	//! It is only mandatory to call UpdateNavMeshPointerBeforeProcessing if 
	//! 1) We will use the NavMesh pointer.
	//! 2) And if the NavMeshQueryProcessing will be performed across multiple frames (ie: Time Sliced Queries). This is because the NavMesh could get invalidated between processing operations.
	//! Even if our NavMeshQueryProcessing is currently only used for Instant queries, we don't know if in the future will be used for Time Sliced queries.
	//! For this reason, we highly recommend to inherit from this class and call UpdateNavMeshPointerBeforeProcessing() to make the code future proof
	class CNavMeshQueryProcessing : public CNavMeshQueryProcessingBase
	{
	protected:
		//! Should be called right before starting processing to make sure the NavMesh pointer is updated and valid
		bool                   UpdateNavMeshPointerBeforeProcessing();

		CNavMeshQueryProcessing(const NavigationMeshID meshId)
			: CNavMeshQueryProcessingBase()
			, m_meshId(meshId)
			, m_pNavMesh(nullptr)
		{
			// Empty
		}


		const NavigationMeshID m_meshId;
		const CNavMesh*        m_pNavMesh;
	};

	//! CTriangleAtQueryProcessing process the provided triangles to find the triangle that contains a given position
	class CTriangleAtQueryProcessing final : public CNavMeshQueryProcessing
	{
	public:
		CTriangleAtQueryProcessing(const NavigationMeshID meshId, const vector3_t& localPosition)
			: CNavMeshQueryProcessing(meshId)
			, m_location(localPosition)
			, m_closestID()
			, m_distMinSq(std::numeric_limits<real_t::unsigned_overflow_type>::max())
		{
			// Empty
		}

		const TriangleID                         GetTriangleId() const { return m_closestID; }

		virtual INavMeshQueryProcessing::EResult Process() override;

	private:
		vector3_t                                m_location;
		TriangleID                               m_closestID;
		real_t::unsigned_overflow_type           m_distMinSq;
	};

	//! CNearestTriangleQueryProcessing process the provided triangles to find the triangle that is closest to a given position.
	//! As opposed to before, the triangle may or may not contain the given position.
	class CNearestTriangleQueryProcessing final : public CNavMeshQueryProcessing
	{
	public:
		CNearestTriangleQueryProcessing(const NavigationMeshID meshId, const vector3_t& localFromPosition, const real_t maxDistance = real_t::max())
			: CNavMeshQueryProcessing(meshId)
			, m_localFromPosition(localFromPosition)
			, m_distMinSq(maxDistance.sqr())
			, m_closestID()
			, m_closestPos(real_t::max())
		{}

		const vector3_t&                         GetClosestPosition() const { return m_closestPos; }
		const real_t                             GetClosestDistance() const { return real_t::sqrtf(m_distMinSq); }
		const TriangleID                         GetClosestTriangleId() const { return m_closestID; }

		virtual INavMeshQueryProcessing::EResult Process() override;

	private:
		const vector3_t                          m_localFromPosition;

		TriangleID                               m_closestID;
		vector3_t                                m_closestPos;
		real_t::unsigned_overflow_type           m_distMinSq;
	};

	//! CGetMeshBordersQueryProcessing process the provided triangles to build
	//! an array containing NavMesh borders (v0 to v1) and the normal of the border pointing out
	class CGetMeshBordersQueryProcessing final : public CNavMeshQueryProcessing
	{
	public:
		CGetMeshBordersQueryProcessing(const NavigationMeshID meshId, const INavMeshQueryFilter& annotationFilter)
			: CNavMeshQueryProcessing(meshId)
			, m_annotationFilter(annotationFilter)
		{
		}

		const INavigationSystem::NavMeshBorderWithNormalArray& GetBordersNormals() const { return m_bordersNormals; }

		virtual INavMeshQueryProcessing::EResult               Process() override;

	private:
		const vector3_t                                        m_localFromPosition;
		const INavMeshQueryFilter&                             m_annotationFilter;

		INavigationSystem::NavMeshBorderWithNormalArray        m_bordersNormals;
	};

	//! CFindTriangleQueryProcessing process the provided triangles to find a specific triangle id.
	//! As soon as it finds it, processing stops
	class CFindTriangleQueryProcessing final : public CNavMeshQueryProcessing
	{
	public:
		CFindTriangleQueryProcessing(const NavigationMeshID meshId, const TriangleID triangleId, const vector3_t& location)
			: CNavMeshQueryProcessing(meshId)
			, m_triangleId(triangleId)
			, m_location(location)
			, m_bFound(false)
		{}

		bool                                     WasTriangleFound() const { return m_bFound; }

		virtual INavMeshQueryProcessing::EResult Process() override;

	private:
		const TriangleID                         m_triangleId;
		const vector3_t                          m_location;
		bool                                     m_bFound;
	};

	//! CTriangleCenterInMeshQueryProcessing process the provided triangles retrieving its center position on the NavMesh and storing it.
	class CTriangleCenterInMeshQueryProcessing final : public CNavMeshQueryProcessing
	{
	public:
		CTriangleCenterInMeshQueryProcessing(const NavigationMeshID meshId)
			: CNavMeshQueryProcessing(meshId)
		{
		}

		const DynArray<Vec3>&                    GetTriangleCenterArray() const { return m_triangleCenterArray; }

		virtual INavMeshQueryProcessing::EResult Process() override;

	private:
		DynArray<Vec3>                           m_triangleCenterArray;
	};
}