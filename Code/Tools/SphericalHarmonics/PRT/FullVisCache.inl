/*
	full visibility cache inline implementations for ray casting 
*/

#if defined(OFFLINE_COMPUTATION)

inline const bool NSH::CFullVisCache::IsUpperHemisphereVisible()const
{
	return m_UpperHemisphereIsVisible;
}

inline const bool NSH::CFullVisCache::IsLowerHemisphereVisible()const
{
	return m_LowerHemisphereIsVisible;
}

inline void NSH::CFullVisCache::CSimpleHit::SetupRay(const Vec3& crRayDir, const Vec3& crRayOrigin, const float cRayLen, const float cSign, const Vec3& crNormal, const float cBias)
{
	NSH::CEveryObjectOnlyOnce::SetupRay(crRayDir, crRayOrigin, cRayLen, cBias);
	m_PNormal = crNormal;
	m_D = cSign;
	m_HasHit = false;
};

inline const bool NSH::CFullVisCache::CSimpleHit::IsIntersecting() const 
{ 
	return m_HasHit; 
}

inline const bool NSH::CFullVisCache::IsFullyVisible(const TVec& crDir)const
{
	if(crDir.z >= 0)
		return m_UpperHemisphereIsVisible;
	else
		return m_LowerHemisphereIsVisible;
}

#endif