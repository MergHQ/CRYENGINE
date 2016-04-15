// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
* Created by Nicolas Schulz, August 2010

   =============================================================================*/

#pragma once

#ifndef _GPUTIMER_H
	#define _GPUTIMER_H

class CSimpleGPUTimer
{
public:
	static void EnableTiming();
	static void DisableTiming();

	static void AllowTiming();
	static void DisallowTiming();

	static bool IsTimingEnabled() { return s_bTimingEnabled; }
	static bool IsTimingAllowed() { return s_bTimingAllowed; }

	static void BeginMeasuring();
	static void EndMeasuring(bool sync);

protected:
	static bool s_bTimingEnabled;
	static bool s_bTimingAllowed;

	#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO) && !(CRY_USE_DX12)
	static DeviceTimestampHandle m_pQueryFreq;
	#endif

	#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
	static bool   m_bDisjoint;
	static UINT64 m_Frequency;
	#endif

public:
	CSimpleGPUTimer();
	~CSimpleGPUTimer();

	void          Start();
	void          Stop();
	void          UpdateTime();

	float         GetTime()         { return m_time; }
	float         GetSmoothedTime() { return m_smoothedTime; }

	float         GetWindowedMaxTime();
	float         GetWindowedMinTime();
	float         GetWindowedMedTime();
	float         GetWindowedAvgTime();
	void          GetWindowedTimes(float& minTime, float& maxTime, float& medTime, float& avgTime);
	unsigned char GetWindowedHistogram(float maxTime, uint buckets, unsigned char* freqs);

	bool          IsStarted() const         { return m_bStarted; }
	bool          HasPendingQueries() const { return m_bWaiting; }

	//protected:
	void Release();
	bool Init();

protected:
	float m_time;
	float m_smoothedTime;

	float m_timeWindow[256];
	int   m_windowPeak;
	int   m_windowPos;

	bool  m_bInitialized;
	bool  m_bStarted;
	bool  m_bWaiting;

	#if (CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO)
	DeviceTimestampHandle m_pQueryStart, m_pQueryStop;
	#elif CRY_PLATFORM_ORBIS
	CGPUTimer*            m_pTimer;
	#endif
};

#endif
