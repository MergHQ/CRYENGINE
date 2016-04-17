/*
	ray casting cache implementation
*/

#if defined(OFFLINE_COMPUTATION)

#include "RasterCube.h"


inline NSH::CRayCache::CRayCache()
{
}

inline void NSH::CRayCache::Reset()
{
	//mark with no hit storage
	m_LastIntersectionObjects.resize(0);
}

inline void NSH::CRayCache::RecordHit(const RasterCubeUserT* cpInObject)
{
	m_LastIntersectionObjects.push_back(cpInObject);
}

template <class THitSink>
const bool NSH::CRayCache::CheckCache(THitSink& rSink)const
{
	if(!m_LastIntersectionObjects.empty())
	{
		float dist = std::numeric_limits<float>::max();
		const TRasterCubeUserTPtrVec::const_iterator cEnd = m_LastIntersectionObjects.end();
		for(TRasterCubeUserTPtrVec::const_iterator iter = m_LastIntersectionObjects.begin(); iter != cEnd; ++iter)
		{
			//return only true if all recorded hits intersect
			if(rSink.ReturnElement(*(*iter), dist) == RETURN_SINK_FAIL)
			{
				//didnt hit
				rSink.ResetHits();//reset to as if nothing was tested
				return false;
			}
		}
		return true;//all hits have hit again
	}
	return false;
}

#endif