// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#if !defined(CRY_USE_DX12_NATIVE) && !defined(CRY_PLATFORM_ORBIS)

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
		uint64 duration = std::max(m_timeValues[timestamp0], m_timeValues[timestamp1]) - std::min(m_timeValues[timestamp0], m_timeValues[timestamp1]);
		return duration / (float)(m_frequency / 1000);
	}

protected:
	uint32                                    m_numTimestamps;
	
	std::array<ID3D11Query*, kMaxTimestamps>  m_timestampQueries;
	ID3D11Query*                              m_pDisjointQuery;

	UINT64                                    m_frequency;
	std::array<uint64, kMaxTimestamps>        m_timeValues;
};

#endif