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

	uint32 IssueTimestamp();
	bool ResolveTimestamps();
	
	float GetTimeMS(uint32 timestamp0, uint32 timestamp1)
	{
		return (m_timeValues[std::max(timestamp0, timestamp1)] - m_timeValues[std::min(timestamp0, timestamp1)]) / (float)(m_frequency / 1000);
	}

protected:
	uint32                                    m_numTimestamps;
	
	std::array<ID3D11Query*, kMaxTimestamps>  m_timestampQueries;
	ID3D11Query*                              m_pDisjointQuery;

	UINT64                                    m_frequency;
	std::array<uint64, kMaxTimestamps>        m_timeValues;
};

#endif