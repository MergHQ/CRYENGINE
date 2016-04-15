// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description:	AI Navigation Interface.

   -------------------------------------------------------------------------
   History:
   - 3:3:2010		:	Created by Kevin Kirst

*************************************************************************/

#ifndef _INAVIGATION_H_
#define _INAVIGATION_H_

#include <CryAISystem/IAISystem.h> // <> required for Interfuscator

struct IAIObject;

struct INavigation
{
	enum EIFMode {IF_AREASBOUNDARIES, IF_AREAS, IF_BOUNDARIES};

	// <interfuscator:shuffle>
	virtual ~INavigation() {}

	virtual uint32 GetPath(const char* szPathName, Vec3* points, uint32 maxpoints) const = 0;
	virtual float  GetNearestPointOnPath(const char* szPathName, const Vec3& vPos, Vec3& vResult, bool& bLoopPath, float& totalLength) const = 0;
	virtual void   GetPointOnPathBySegNo(const char* szPathName, Vec3& vResult, float segNo) const = 0;
	virtual bool   IsSegmentValid(IAISystem::tNavCapMask navCap, float rad, const Vec3& posFrom, Vec3& posTo, IAISystem::ENavigationType& navTypeFrom) const = 0;

	//! If there's intersection vClosestPoint indicates the intersection point, and the edge normal is optionally returned.
	//! \param bForceNormalOutwards If set then in the case of forbidden boundaries this normal is chosen to point (partly) towards vStart.
	//! \param nameToSkip Can optionally point to a string indicating a forbidden area area to not check.
	//! \param mode Indicates if areas and/or boundaries should be checked.
	virtual bool IntersectsForbidden(const Vec3& vStart, const Vec3& vEnd, Vec3& vClosestPoint, const string* nameToSkip = 0, Vec3* pNormal = NULL, INavigation::EIFMode mode = INavigation::IF_AREASBOUNDARIES, bool bForceNormalOutwards = false) const = 0;

	//! Returns true if it is impossible to get from start to end without crossing a forbidden boundary.
	//! Moving out of a forbidden region is acceptable.
	//! Assumes path finding is OK.
	virtual bool IsPathForbidden(const Vec3& start, const Vec3& end) const = 0;

	//! Returns true if a point is inside forbidden boundary/area edge or close to its edge.
	virtual bool IsPointForbidden(const Vec3& pos, float tol, Vec3* pNormal = 0) const = 0;

	//! Get the best point outside any forbidden region given the input point, and optionally a start position to stay close to.
	virtual Vec3 GetPointOutsideForbidden(Vec3& pos, float distance, const Vec3* startPos = 0) const = 0;

	//! Returns nearest designer created path/shape.
	//! \param devalue Specifies how long the path will be unusable by others after the query.
	//! \param useStartNode If true the start point of the path is used to select nearest path instead of the nearest point on path.
	virtual const char* GetNearestPathOfTypeInRange(IAIObject* requester, const Vec3& pos, float range, int type, float devalue, bool useStartNode) = 0;

	//! Modifies the additional cost multiplier of a named cost nav modifier.
	//! If factor < 0 then the cost is made infinite.
	//! If >= 0 then the cost is multiplied by 1 + factor.
	//! The original value gets reset when leaving/entering game mode etc.
	virtual void ModifyNavCostFactor(const char* navModifierName, float factor) = 0;

	//! Returns the names of the region files generated during volume generation.
	virtual void GetVolumeRegionFiles(const char* szLevel, const char* szMission, DynArray<CryStringT<char>>& filenames) const = 0;
	// </interfuscator:shuffle>
};

#endif //_INAVIGATION_H_
