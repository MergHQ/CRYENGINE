// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "CryProfilingSystemSharedImpl.h"

#if ALLOW_BROFILER

class CBrofiler : public CCryProfilingSystemImpl
{
public:
	void PauseRecording(bool pause) final {}
	bool IsPaused() const final { return false; }

	void DescriptionCreated(SProfilingDescription*) final;
	void DescriptionDestroyed(SProfilingDescription*) final;
	
	void StartFrame() final {}
	void EndFrame() final {}

	void RegisterCVars() final {}
	void UnregisterCVars() final {}

	static SSystemGlobalEnvironment::TProfilerSectionEndCallback StartSectionStatic(SProfilingSection*);
	static void RecordMarkerStatic(SProfilingMarker*) {}

private:
	static void EndSectionStatic(SProfilingSection*);
};

#endif