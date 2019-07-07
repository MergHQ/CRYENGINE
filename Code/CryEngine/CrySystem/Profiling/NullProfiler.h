// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryProfilingSystemSharedImpl.h"

class CNullProfiler final : public CCryProfilingSystemImpl
{
public:
	void PauseRecording(bool pause) override {}
	bool IsPaused() const override { return false; }

	void StartFrame() override {}
	void EndFrame() override {}

	void RegisterCVars() override {}
	void UnregisterCVars() override {}

	static SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSectionStatic(SProfilingSection*) { return nullptr; }
	static void RecordMarkerStatic(SProfilingMarker*) {}
};
