// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

template <class Impl> inline void CDeviceTimestampGroup_Base<Impl>::Init()
{
	((Impl*)this)->Init();
}

template <class Impl> inline void CDeviceTimestampGroup_Base<Impl>::BeginMeasurement()
{
	((Impl*)this)->BeginMeasurement();
}

template <class Impl> inline void CDeviceTimestampGroup_Base<Impl>::EndMeasurement()
{
	((Impl*)this)->EndMeasurement();
}

template <class Impl> inline uint32 CDeviceTimestampGroup_Base<Impl>::IssueTimestamp()
{
	return ((Impl*)this)->IssueTimestamp();
}

template <class Impl> inline bool CDeviceTimestampGroup_Base<Impl>::ResolveTimestamps()
{
	return ((Impl*)this)->ResolveTimestamps();
}

template <class Impl> inline float CDeviceTimestampGroup_Base<Impl>::GetTimeMS(uint32 timestamp0, uint32 timestamp1)
{
	return ((Impl*)this)->GetTimeMS(timestamp0, timestamp1);
}