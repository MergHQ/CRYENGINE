// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if defined(CRY_USE_DX12_NATIVE)

class CDeviceTimestampGroup : public CDeviceTimestampGroup_Base<CDeviceTimestampGroup>
{
public:
	CDeviceTimestampGroup();
	~CDeviceTimestampGroup();
	
	void Init();

	void BeginMeasurement();
	void EndMeasurement();

	uint32 IssueTimestamp();
	bool ResolveTimestamps();

	float GetTimeMS(uint32 timestamp0, uint32 timestamp1)
	{
		timestamp0 -= m_groupIndex * kMaxTimestamps;
		timestamp1 -= m_groupIndex * kMaxTimestamps;
		return (m_timeValues[std::max(timestamp0, timestamp1)] - m_timeValues[std::min(timestamp0, timestamp1)]) / (float)(m_frequency / 1000);
	}

protected:
	uint32                              m_numTimestamps;
	uint32                              m_groupIndex;

	DeviceFenceHandle                   m_fence;
	UINT64                              m_frequency;
	std::array<uint64, kMaxTimestamps>  m_timeValues;

protected:
	static bool                         s_reservedGroups[4];
};

#endif