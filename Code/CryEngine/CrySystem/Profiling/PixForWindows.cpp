// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PixForWindows.h"

#if WIN_PIX_AVAILABLE

#include <CryThreading/IThreadManager.h>
#include <CryString/StringUtils.h>

// https://devblogs.microsoft.com/pix/winpixeventruntime/
#ifdef ENABLE_PROFILING_CODE
#	define USE_PIX
#endif
#include <WinPixEventRuntime/pix3.h>

REGISTER_PROFILER(CPixForWindows, "PIX", "-pix4windows");

void CPixForWindows::PauseRecording(bool pause) {}
bool CPixForWindows::IsPaused() const { return false; }

SSystemGlobalEnvironment::TProfilerSectionEndCallback CPixForWindows::StartSectionStatic(SProfilingSection* pSection)
{
	if(pSection->szDynamicName && pSection->szDynamicName[0])
		PIXBeginEvent(pSection->pDescription->color_argb, "%s\n%s", pSection->pDescription->szEventname, pSection->szDynamicName);
	else
		PIXBeginEvent(pSection->pDescription->color_argb, pSection->pDescription->szEventname);
	return &EndSectionStatic;
}

void CPixForWindows::EndSectionStatic(SProfilingSection*)
{
	PIXEndEvent();
}

void CPixForWindows::RecordMarkerStatic(SProfilingMarker* pMarker)
{
	PIXSetMarker(pMarker->pDescription->color_argb, pMarker->pDescription->szEventname);
}

void CPixForWindows::StartFrame()
{
	PIXBeginEvent(PIX_COLOR_INDEX(0), "Frame %d", gEnv->nMainFrameID);
}

void CPixForWindows::EndFrame()
{
	PIXEndEvent();
}

#endif