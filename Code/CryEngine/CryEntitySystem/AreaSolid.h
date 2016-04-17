// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   AreaSolid.h
//  Version:     v1.00
//  Created:     21/Nov/2011 by Jaesik.
//  Compilers:   Visual Studio 2010
//  Description:
//	The AreaSolid has most general functions for an area object.
////////////////////////////////////////////////////////////////////////////

#ifndef __AREASOLID_H_
#define __AREASOLID_H_

#include "AreaUtil.h"

class CBSPTree3D;
class CSegmentSet;

class CAreaSolid
{

public:

	enum ESegmentType
	{
		eSegmentType_Open,
		eSegmentType_Close,
	};

	enum ESegmentQueryFlag
	{
		eSegmentQueryFlag_Obstruction         = 0x0001,
		eSegmentQueryFlag_Open                = 0x0002,
		eSegmentQueryFlag_All                 = eSegmentQueryFlag_Obstruction | eSegmentQueryFlag_Open,

		eSegmentQueryFlag_UsingReverseSegment = 0x0010
	};

public:

	CAreaSolid();
	~CAreaSolid()
	{
		Clear();
	}

	void AddRef()
	{
		CryInterlockedIncrement(&m_nRefCount);
	}
	void Release()
	{
		if (CryInterlockedDecrement(&m_nRefCount) <= 0)
			delete this;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////
	bool QueryNearest(const Vec3& vPos, int queryFlag, Vec3& outNearestPos, float& outNearestDistance) const;
	bool IsInside(const Vec3& vPos) const;
	void Draw(const Matrix34& worldTM, const ColorB& color0, const ColorB& color1) const;

	/////////////////////////////////////////////////////////////////////////////////////////////
	void AddSegment(const Vec3* verticesOfConvexhull, bool bObstruction, int numberOfPoints);
	void BuildBSP();

	////////////////////////////////////////////////////////////////////////////////////////////
	const AABB& GetBoundBox()
	{
		return m_BoundBox;
	}

	void GetMemoryUsage(ICrySizer* pSizer) const;

private:

	void        Clear();

	static bool IsQueryTypeIdenticalToSegmentType(int queryFlag, ESegmentType segmentType)
	{
		if ((queryFlag & eSegmentQueryFlag_All) != eSegmentQueryFlag_All)
		{
			if ((queryFlag & eSegmentQueryFlag_Obstruction) && segmentType != eSegmentType_Close)
				return false;
			if ((queryFlag & eSegmentQueryFlag_Open) && segmentType != eSegmentType_Open)
				return false;
		}
		return true;
	}

private:

	CBSPTree3D*               m_BSPTree;
	std::vector<CSegmentSet*> m_SegmentSets;
	AABB                      m_BoundBox;
	int                       m_nRefCount;

};

#endif // __AREASOLID_H_
