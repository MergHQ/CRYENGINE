/*
	ray casting cache interface 
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <PRT/PRTTypes.h>
#include "RasterCube.h"

class CSimpleIndexedMesh;

namespace NSH
{
	struct RasterCubeUserT
	{
		INSTALL_CLASS_NEW(RasterCubeUserT)

		Vec3 vertices[3];					//!< vertices of triangle for fast access
		Vec3 vNormal;							//!< plane normal of triangle
		const CSimpleIndexedMesh* pMesh;	//!< mesh triangle belonging to
		uint32 faceID;						//!< face id within mesh
		bool fastProcessing : 1;	//!< true if it belongs to a opaque material and no interreflections are required, enables possible early out of ray tracing
		bool isSingleSided : 1;		//!< true if it does not belong to a 2 sided material 

		bool operator == (const RasterCubeUserT& A) const
		{ return pMesh == A.pMesh && faceID == A.faceID; }
		bool operator > (const RasterCubeUserT& A) const
		{ 
			if(pMesh == A.pMesh)
				return faceID > A.faceID;
			else
				return (size_t)pMesh > (size_t)A.pMesh;
		}
	}; 

//	typedef CRasterCube<RasterCubeUserT, true> TRasterCubeImpl;	//!< rastercube with 3 raster tables
	typedef CRasterCube<RasterCubeUserT, false> TRasterCubeImpl;	//!< rastercube without X raster table

	typedef std::vector<const RasterCubeUserT*, CSHAllocator<const RasterCubeUserT*> > TRasterCubeUserTPtrVec;

	/************************************************************************************************************************************************/

	static const uint32 gscAlreadyTestedArraySize = 512;	//!< static array size for rastercube sink

	//!< local classes for rastercube impl
	class CEveryObjectOnlyOnce :public CPossibleIntersectionSink<RasterCubeUserT>
	{
	public:
		void SetupRay( const Vec3& vRayDir, const Vec3& vRayOrigin, const float cRayLen, const float cBias)	//!< ray setup
		{
			m_RayDir = vRayDir;
			m_RayOrigin = vRayOrigin;
			m_RayLen = cRayLen;
			m_AlreadyTested=0;
			m_Bias = cBias;
		}

	protected:
		//!< [0..gscAlreadyTestedArraySize-1], this is faster than a set<> at least for typical amounts
		const RasterCubeUserT*	m_arrAlreadyTested[gscAlreadyTestedArraySize];		
		Vec3					m_RayOrigin;									//!< position in world space 
		Vec3					m_RayDir;											//!< direction in world space
		uint32				m_AlreadyTested;							//!< 0..gscAlreadyTestedArraySize
		float					m_RayLen;											//!< length of ray
		float					m_Bias;

		//! /return true=object is already tested, false otherwise
		const bool InsertAlreadyTested(const RasterCubeUserT *cpInObject)
		{
			for(int i=m_AlreadyTested-1; i>=0; --i)
			{
				if(m_arrAlreadyTested[i] == cpInObject)
					return true;
			}
			if(m_AlreadyTested < gscAlreadyTestedArraySize-1) 
				m_arrAlreadyTested[m_AlreadyTested++] = cpInObject;
			else
				GetSHLog().LogError("Rastercube overflow in InsertAlreadyTested\n");
			return false;
		}
	};

	/************************************************************************************************************************************************/

	//!< cache manager for ray casting, since there are multiple call possibilities, we need a central class here
	//!< special is, that it is only used for the same ray source vector
	//!< supports multiple hits
	class CRayCache
	{
	public:

		CRayCache();				//!< standard constructor
		void Reset();				//!< resets it to no cache record and to a new ray source
		void RecordHit(const RasterCubeUserT* cpInObject);//!< records a hit
		//!< returns true if the ray is from the same source and intersects the same way (same intersection count and so on)
		template <class THitSink>
		const bool CheckCache(THitSink& rSink)const;	

	private:
		TRasterCubeUserTPtrVec m_LastIntersectionObjects;		//!< cached hit object
	};
}

#include "RayCache.inl"

#endif