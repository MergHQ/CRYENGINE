// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
* Created by Nicolas Schulz, August 2010

   =============================================================================*/

#include "StdAfx.h"
#include "GPUTimer.h"
#include "DriverD3D.h"

bool CSimpleGPUTimer::s_bTimingEnabled = false;
bool CSimpleGPUTimer::s_bTimingAllowed = true;

#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
DeviceTimestampHandle CSimpleGPUTimer::m_pQueryFreq = 0;
#endif

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
bool CSimpleGPUTimer::m_bDisjoint = false;
UINT64 CSimpleGPUTimer::m_Frequency = 0;
#endif

void CSimpleGPUTimer::EnableTiming()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (!s_bTimingEnabled && s_bTimingAllowed)
	{
		s_bTimingEnabled = true;

	#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
		gcpRendD3D->m_DevMan.CreateTimestamp(m_pQueryFreq, true);
	#endif

	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
		// Set default values initially, in case we don't wait for DISJOINT queries later and they never become ready
		m_bDisjoint = false;
		gcpRendD3D->m_DevMan.GetTimestampFrequency(&m_Frequency);
	#endif
	}
#endif
}

void CSimpleGPUTimer::DisableTiming()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (s_bTimingEnabled)
	{
		s_bTimingEnabled = false;

	#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
		gcpRendD3D->m_DevMan.ReleaseTimestamp(m_pQueryFreq);
	#endif
	}
#endif
}

void CSimpleGPUTimer::AllowTiming()
{
	s_bTimingAllowed = true;
}

void CSimpleGPUTimer::DisallowTiming()
{
	s_bTimingAllowed = false;
	if (gcpRendD3D->m_pPipelineProfiler)
		gcpRendD3D->m_pPipelineProfiler->ReleaseGPUTimers();
	DisableTiming();
}

void CSimpleGPUTimer::BeginMeasuring()
{
	if (s_bTimingEnabled)
	{
#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
		gcpRendD3D->m_DevMan.IssueTimestamp(m_pQueryFreq, true);
#endif
	}
}

void CSimpleGPUTimer::EndMeasuring(bool sync)
{
	if (s_bTimingEnabled)
	{
#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
		gcpRendD3D->m_DevMan.IssueTimestamp(m_pQueryFreq, false);
#endif

		gcpRendD3D->m_DevMan.ResolveTimestamps(sync, sync);

#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT sData;
		if (gcpRendD3D->m_DevMan.SyncTimestamp(m_pQueryFreq, &sData, true) == S_OK)
		{
			m_bDisjoint = sData.Disjoint ? true : false;
			m_Frequency = sData.Frequency;
		}
#elif (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO)
		gcpRendD3D->m_DevMan.GetTimestampFrequency(&m_Frequency);
#endif
	}
}

CSimpleGPUTimer::CSimpleGPUTimer()
	: m_time(0.0f)
	, m_smoothedTime(0.0f)
	, m_windowPeak(0)
	, m_windowPos(0)
	, m_bInitialized(false)
	, m_bStarted(false)
	, m_bWaiting(false)
#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
	, m_pQueryStart(0)
	, m_pQueryStop(0)
#elif CRY_PLATFORM_ORBIS
	, m_pTimer(nullptr)
#endif
{
}

CSimpleGPUTimer::~CSimpleGPUTimer()
{
	Release();
}

void CSimpleGPUTimer::Release()
{
#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
	gcpRendD3D->m_DevMan.ReleaseTimestamp(m_pQueryStart);
	gcpRendD3D->m_DevMan.ReleaseTimestamp(m_pQueryStop);
#elif CRY_PLATFORM_ORBIS
	DXOrbis::Device()->ReleaseGPUTimer(m_pTimer);
#endif

	m_bInitialized = false;
	m_bWaiting = false;
	m_bStarted = false;
	m_smoothedTime = 0.f;
}

bool CSimpleGPUTimer::Init()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (!m_bInitialized)
	{
	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
		m_bInitialized =
		  gcpRendD3D->m_DevMan.CreateTimestamp(m_pQueryStart, false) == S_OK &&
		  gcpRendD3D->m_DevMan.CreateTimestamp(m_pQueryStop, false) == S_OK;
	#elif CRY_PLATFORM_ORBIS
		m_pTimer = DXOrbis::Device()->CreateGPUTimer("CSimpleGPUTimer");
		m_bInitialized = true;
	#endif
	}
#endif

	return m_bInitialized;
}

void CSimpleGPUTimer::Start()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (s_bTimingEnabled && !m_bWaiting && Init())
	{
	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
		m_bStarted = gcpRendD3D->m_DevMan.IssueTimestamp(m_pQueryStart, false) == S_OK;
	#elif CRY_PLATFORM_ORBIS
		DXOrbis::Device()->PushTimer(m_pTimer);
		m_bStarted = true;
	#endif
	}
#endif
}

void CSimpleGPUTimer::Stop()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (s_bTimingEnabled && m_bStarted && m_bInitialized)
	{
	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
		m_bStarted = false;
		m_bWaiting = gcpRendD3D->m_DevMan.IssueTimestamp(m_pQueryStop, false) == S_OK;
	#elif CRY_PLATFORM_ORBIS
		DXOrbis::Device()->PopTimer(m_pTimer);
		m_bStarted = false;
		m_bWaiting = true;
	#endif
	}
#endif
}

void CSimpleGPUTimer::UpdateTime()
{
#ifdef ENABLE_SIMPLE_GPU_TIMERS
	if (s_bTimingEnabled && m_bWaiting && m_bInitialized)
	{
	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
		UINT64 timeStart, timeStop;

		m_bWaiting =
		  gcpRendD3D->m_DevMan.SyncTimestamp(m_pQueryStart, &timeStart, false) != S_OK ||
		  gcpRendD3D->m_DevMan.SyncTimestamp(m_pQueryStop, &timeStop, false) != S_OK;

		if (!m_bDisjoint)
		{
			float time = (timeStop - timeStart) / (float)(m_Frequency / 1000);
			// Filter out wrong insane values that get reported sometimes
			if (time >= 0.f && time < 1000.f)
				m_time = time;
		}

	#elif CRY_PLATFORM_ORBIS
		m_time = 1000.0f * DXOrbis::Device()->GetTimerTime(m_pTimer);
		m_bWaiting = false;
	#endif

		m_smoothedTime = m_smoothedTime * 0.7f + m_time * 0.3f;
	}
	else
	{
		// Reset timers when not used in frame
		m_time = 0.0f;
		m_smoothedTime = 0.0f;
	}

	m_timeWindow[m_windowPos] = m_time;
	m_windowPos = (m_windowPos + 1) & 0xFF;
	m_windowPeak = std::max(m_windowPeak, m_windowPos);
#endif
}

float CSimpleGPUTimer::GetWindowedMaxTime()
{
	float maxTime = 0.0f;
	for (int w = 0; w < m_windowPeak; ++w)
	{
		maxTime = std::max(maxTime, m_timeWindow[w]);
	}
	return maxTime;
}

float CSimpleGPUTimer::GetWindowedMinTime()
{
	float minTime = FLT_MAX;
	for (int w = 0; w < m_windowPeak; ++w)
	{
		minTime = std::min(minTime, m_timeWindow[w]);
	}
	return minTime;
}

float CSimpleGPUTimer::GetWindowedMedTime()
{
	float minTime = FLT_MAX;
	float maxTime = 0.0f;
	for (int w = 0; w < m_windowPeak; ++w)
	{
		minTime = std::min(minTime, m_timeWindow[w]);
		maxTime = std::max(maxTime, m_timeWindow[w]);
	}
	return (maxTime + minTime) * 0.5f;
}

float CSimpleGPUTimer::GetWindowedAvgTime()
{
	float avgTime = 0.0f;
	for (int w = 0; w < m_windowPeak; ++w)
	{
		avgTime += m_timeWindow[w];
	}
	return avgTime / m_windowPeak;
}

void CSimpleGPUTimer::GetWindowedTimes(float& minTime, float& maxTime, float& medTime, float& avgTime)
{
	float _minTime = FLT_MAX;
	float _maxTime = 0.0f;
	float _avgTime = 0.0f;
	for (int w = 0; w < m_windowPeak; ++w)
	{
		_minTime = std::min(_minTime, m_timeWindow[w]);
		_maxTime = std::max(_maxTime, m_timeWindow[w]);
		_avgTime += m_timeWindow[w];
	}
	minTime = _minTime;
	maxTime = _maxTime;
	medTime = (_maxTime + _minTime) * 0.5f;
	avgTime = _avgTime / m_windowPeak;
}

unsigned char CSimpleGPUTimer::GetWindowedHistogram(float maxTime, uint buckets, unsigned char* freqs)
{
	unsigned char maxFreq = 0;
	memset(freqs, 0, sizeof(unsigned char) * buckets);
	for (int w = 0; w < m_windowPeak; ++w)
	{
		int bucket = int(ceilf((m_timeWindow[w] * (buckets - 1) + 0.5f) / maxTime));
		if (bucket < buckets)
			++freqs[bucket];
	}
	for (uint b = 0; b < buckets; ++b)
	{
		maxFreq = std::max(maxFreq, freqs[b]);
	}
	return maxFreq;
}
