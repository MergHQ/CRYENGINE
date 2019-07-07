// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "CryProfilingSystemSharedImpl.h"

class CPixForWindows : public CCryProfilingSystemImpl
{
public:
	void PauseRecording(bool pause) final;
	bool IsPaused() const final;

	void RegisterCVars() final {};
	void UnregisterCVars() final {};

	void StartFrame() final;
	void EndFrame() final;

	static SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSectionStatic(SProfilingSection*);
	static void EndSectionStatic(SProfilingSection*);
	static void RecordMarkerStatic(SProfilingMarker*);
};