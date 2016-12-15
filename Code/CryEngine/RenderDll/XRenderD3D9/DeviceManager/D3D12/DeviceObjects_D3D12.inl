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

	uint32 IssueTimestamp(void* pCommandList);
	bool ResolveTimestamps();

	float GetTimeMS(uint32 timestamp0, uint32 timestamp1)
	{
		timestamp0 -= m_groupIndex * kMaxTimestamps;
		timestamp1 -= m_groupIndex * kMaxTimestamps;
		
		uint64 duration = std::max(m_timeValues[timestamp0], m_timeValues[timestamp1]) - std::min(m_timeValues[timestamp0], m_timeValues[timestamp1]);
		return duration / (float)(m_frequency / 1000);
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