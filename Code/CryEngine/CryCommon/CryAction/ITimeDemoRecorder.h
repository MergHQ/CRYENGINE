// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   Description:
   Provides interface for TimeDemo (CryAction)
   Can attach listener to perform game specific stuff on Record/Play
*************************************************************************/

#pragma once

struct STimeDemoFrameInfo
{
	int   nPolysRendered;
	float fFrameRate;
	int   nDrawCalls;
};

struct STimeDemoInfo
{
	std::vector<STimeDemoFrameInfo> frames;

	float               lastPlayedTotalTime = 0.0f;
	float               lastAveFrameRate = 0.0f;
	float               minFPS = 0.0f;
	float               maxFPS = 0.0f;
	uint32              minFPS_Frame = 0;
	uint32              maxFPS_Frame = 0;

	//! How many polygons were recorded.
	uint32 nTotalPolysRecorded = 0;

	//! How many polygons were played.
	uint32 nTotalPolysPlayed = 0;
};

struct STimeDemoFrameRecord
{
	Vec3 playerPosition;
	Quat playerRotation;
	Quat playerViewRotation;
	Vec3 hmdPositionOffset;
	Quat hmdViewRotation;
};

struct ITimeDemoListener
{
	virtual ~ITimeDemoListener(){}
	virtual void OnRecord(bool bEnable) = 0;
	virtual void OnPlayback(bool bEnable) = 0;
	virtual void OnPlayFrame() = 0;
};

struct ITimeDemoRecorder
{
	virtual ~ITimeDemoRecorder(){}

	virtual bool           IsRecording() const = 0;
	virtual bool           IsPlaying() const = 0;
	virtual bool           IsChainLoading() const = 0;

	virtual void           RegisterListener(ITimeDemoListener* pListener) = 0;
	virtual void           UnregisterListener(ITimeDemoListener* pListener) = 0;
	virtual void           GetCurrentFrameRecord(STimeDemoFrameRecord& record) const = 0;
	virtual STimeDemoInfo* GetLastPlayedTimeDemo() const = 0;

	virtual void           Reset() = 0;
	virtual void           PreUpdate() = 0;
	virtual void           PostUpdate() = 0;

	//! Called when recorder is registered with GameFramework (\see IGameFramework::SetITimeDemoRecorder)
	virtual void           OnRegistered() = 0;
	//! Called when recorder is unregistered from GameFramework (\see IGameFramework::SetITimeDemoRecorder)
	virtual void           OnUnregistered() = 0;

	virtual void           GetMemoryStatistics(class ICrySizer* pSizer) const = 0;
};
