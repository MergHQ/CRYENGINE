// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Math.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/ITimer.h>

struct SProfilingDescription;
struct SProfilingSection;
struct SProfilingMarker;
typedef int32 TProfilingCount;
typedef int64 TProfilingValue;

enum EProfiledSubsystem : uint8
{
	PROFILE_RENDERER,
	PROFILE_3DENGINE,
	PROFILE_PARTICLE,
	PROFILE_AI,
	PROFILE_ANIMATION,
	PROFILE_MOVIE,
	PROFILE_ENTITY,
	PROFILE_FONT,
	PROFILE_NETWORK,
	PROFILE_PHYSICS,
	PROFILE_SCRIPT,
	PROFILE_SCRIPT_CFUNC,
	PROFILE_AUDIO,
	PROFILE_EDITOR,
	PROFILE_SYSTEM,
	PROFILE_ACTION,
	PROFILE_GAME,
	PROFILE_INPUT,
	PROFILE_LOADING_ONLY, //!< Deprecated

	PROFILE_LAST_SUBSYSTEM //!< Must always be last.
};

struct ICryProfilingSystem
{
	virtual void PauseRecording(bool pause) = 0;
	//! Are we collecting profiling data?
	virtual bool IsPaused() const = 0;

	virtual void DescriptionCreated(SProfilingDescription*) = 0;
	virtual void DescriptionDestroyed(SProfilingDescription*) = 0;

	virtual void StartFrame() = 0;
	virtual void EndFrame() = 0;
	//! Must be called when something quacks like the end of the frame.
	virtual void OnSliceAndSleep() = 0;

	virtual const char* GetModuleName(EProfiledSubsystem) const = 0;
	virtual const char* GetModuleName(const SProfilingSection*) const = 0;

	//! get time lost to profiling in milliseconds, may return 0, if not measured
	virtual float GetProfilingTimeCost() const = 0;
	virtual float GetAverageProfilingTimeCost() const = 0;

protected:
	~ICryProfilingSystem() = default;
};

//! Description of a code section we're profiling.
struct SProfilingDescription
{
	SProfilingDescription(const char* szFilename, const char* szEventname, uint16 line, bool isWaiting, EProfiledSubsystem subsystem)
		: szFilename(szFilename), szEventname(szEventname), line(line), isWaiting(isWaiting), subsystem(subsystem)
		, color_argb(0), customData(0)
	{
		if (gEnv && gEnv->pSystem)
			gEnv->pSystem->GetProfilingSystem()->DescriptionCreated(this);
	}

	~SProfilingDescription()
	{
		if (gEnv && gEnv->pSystem)
			gEnv->pSystem->GetProfilingSystem()->DescriptionDestroyed(this);
	}

	const char* szFilename;
	const char* szEventname;
	uint16 line;
	bool isWaiting;
	EProfiledSubsystem subsystem;
	//! color stored with 1B per channel
	uint32 color_argb;

	//! can be used by a profiling system to attach its own information
	mutable uintptr_t customData;
};

struct SProfilingSection
{
	SProfilingSection(const SProfilingDescription* pDescription, const char* szDynamicName)
		: pDescription(pDescription), szDynamicName(szDynamicName), childExcludeValue(0), customData(0) // startValue is set by the profiling system
	{
		endCallback = gEnv->startProfilingSection(this);
	}

	~SProfilingSection()
	{
		if (endCallback)
			endCallback(this);
	}

	const SProfilingDescription* pDescription;
	//! optional description of the specific instance, e.g. parameters of called function
	const char* szDynamicName;

	TProfilingValue startValue;
	TProfilingValue childExcludeValue;

	//! can be used by a profiling system to attach its own information
	uintptr_t customData;

protected:
	//! upon starting a section the profiling system provides the callback to finish the section
	SSystemGlobalEnvironment::TProfilerSectionEndCallback endCallback;
};

struct SProfilingMarker
{
	SProfilingMarker(const SProfilingDescription* pDescription, const char* szDynamicName) :
		pDescription(pDescription), szDynamicName(szDynamicName), threadId(CryGetCurrentThreadId())
	{
		gEnv->recordProfilingMarker(this);
	}

	const SProfilingDescription* pDescription;
	const char* szDynamicName;
	threadID threadId;
};

#ifdef ENABLE_PROFILING_CODE

#define CRYPROF_CAT_(a, b) a ## b
#define CRYPROF_CAT(a, b)  CRYPROF_CAT_(a, b)

#define CRY_PROFILE_SECTION_FULL(subsystem, szName, szArg, isWaiting) \
	static_assert(szName != nullptr, "Only use string literals as the name of a profiling section!"); \
	static_assert(subsystem >= EProfiledSubsystem(0) || true, "The subsystem cannot be set dynamically!"); \
	static_assert(isWaiting || true, "The waiting status cannot be set dynamically!"); \
	const static SProfilingDescription CRYPROF_CAT(profEventDesc, __LINE__) (__FILE__, szName, uint16(__LINE__), isWaiting, subsystem); \
	SProfilingSection CRYPROF_CAT(profSection, __LINE__)(&CRYPROF_CAT(profEventDesc, __LINE__), szArg);

#define CRY_PROFILE_FUNCTION(subsystem)               CRY_PROFILE_SECTION_FULL(subsystem, __FUNC__, nullptr, false)
#define CRY_PROFILE_FUNCTION_ARG(subsystem, argument) CRY_PROFILE_SECTION_FULL(subsystem, __FUNC__, argument, false)
#define CRY_PROFILE_FUNCTION_WAITING(subsystem)       CRY_PROFILE_SECTION_FULL(subsystem, __FUNC__, nullptr, true)

#define CRY_PROFILE_SECTION(subsystem, szName)            CRY_PROFILE_SECTION_FULL(subsystem, szName, nullptr, false)
#define CRY_PROFILE_SECTION_ARG(subsystem, szName, szArg) CRY_PROFILE_SECTION_FULL(subsystem, szName, szArg, false)
#define CRY_PROFILE_SECTION_WAITING(subsystem, szName)    CRY_PROFILE_SECTION_FULL(subsystem, szName, nullptr, true)

#define CRY_PROFILE_MARKER(subsystem, szName) \
	const static SProfilingDescription CRYPROF_CAT(profMarkerDesc, __LINE__) (__FILE__, szName, __LINE__, false, subsystem); \
	SProfilingMarker CRYPROF_CAT(profMarker, __LINE__)(&CRYPROF_CAT(profMarkerDesc, __LINE__), nullptr);

#define CRY_PROFILE_MARKER_ARG(subsystem, szName, szArg) \
	const static SProfilingDescription CRYPROF_CAT(profMarkerDesc, __LINE__) (__FILE__, szName, __LINE__, false, subsystem); \
	SProfilingMarker CRYPROF_CAT(profMarker, __LINE__)(&CRYPROF_CAT(profMarkerDesc, __LINE__), szArg);

#else

#define CRY_PROFILE_FUNCTION(subsystem)
#define CRY_PROFILE_FUNCTION_ARG(subsystem, szArg)
#define CRY_PROFILE_FUNCTION_WAITING(subsystem)

#define CRY_PROFILE_SECTION(subsystem, szName)
#define CRY_PROFILE_SECTION_ARG(subsystem, szName, szArg)
#define CRY_PROFILE_SECTION_WAITING(subsystem, szName)

#define CRY_PROFILE_MARKER(subsystem, szName)
#define CRY_PROFILE_MARKER_ARG(subsystem, szName, szArg)

#endif

#ifdef ENABLE_LOADING_PROFILER
struct SBootprofilerAutoSession
{
	SBootprofilerAutoSession(const char* szName) { if (gEnv->pSystem) gEnv->pSystem->StartBootProfilerSession(szName); }
	~SBootprofilerAutoSession() { if (gEnv->pSystem) gEnv->pSystem->EndBootProfilerSession(); }
};
#	define LOADING_TIME_PROFILE_AUTO_SESSION(NAME) SBootprofilerAutoSession __bootProfSession(NAME)
#else
#	define LOADING_TIME_PROFILE_AUTO_SESSION(NAME)
#endif
