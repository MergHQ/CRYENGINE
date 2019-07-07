// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/Profilers/ICryProfilingSystem.h>
#include <CrySystem/Profilers/SamplerStats.h>
#include <CryInput/IInput.h>
#include <CryThreading/MultiThread_Containers.h>

//! Extension of the base interface with functionality used only in CrySystem
class CCryProfilingSystemImpl : public ICryProfilingSystem, public IInputEventListener
{
public:
	CCryProfilingSystemImpl();
	virtual ~CCryProfilingSystemImpl() = default;

	//////////////////////////////////////////////////////////
	// ICryProfilingSystem
	//! Must be called when something quacks like the end of the frame.
	void OnSliceAndSleep() final;

	void DescriptionCreated(SProfilingDescription*) override;
	void DescriptionDestroyed(SProfilingDescription*) override;

	float GetProfilingTimeCost() const override { return m_profilingOverhead.Latest(); }
	float GetAverageProfilingTimeCost() const override { return m_profilingOverhead.Average(); }

	const char* GetModuleName(EProfiledSubsystem) const final;
	const char* GetModuleName(const SProfilingSection*) const final;
	//////////////////////////////////////////////////////////

	virtual void RegisterCVars() = 0;
	virtual void UnregisterCVars() = 0;

	std::vector<SProfilingDescription*> ReleaseDescriptions();

	// IInputEventListener
	bool OnInputEvent(const SInputEvent& event) override;
	// ~IInputEventListener

protected:
	uint32 GenerateColorBasedOnName(const char* name);

	// per frame
	CSamplerStats<int64> m_profilingOverhead;
	CSamplerStats<TProfilingCount> m_pageFaultsPerFrame;
	CryMT::vector<SProfilingDescription*> m_descriptions;
};

namespace Cry { 
namespace ProfilerRegistry
{
	typedef CCryProfilingSystemImpl* (*TProfilerFactory)();
	typedef void (*TProfilerThreadCallback) ();

	struct SEntry
	{
		string name;
		string cmdLineArgument;
		TProfilerFactory factory;
		SSystemGlobalEnvironment::TProfilerSectionStartCallback sectionCallback;
		SSystemGlobalEnvironment::TProfilerMarkerCallback markerCallback;
		TProfilerThreadCallback onThreadEntry;
		TProfilerThreadCallback onThreadExit;
	};
	
	namespace Detail
	{
		std::vector<SEntry>& Get();
	}

	// Adds a profiler to the list of known profilers
	// use through REGISTER_PROFILER macro
	template<class TProfiler>
	struct SRegistrar
	{
		SRegistrar(const char* szProfilerName, const char* szClArgument
				, TProfilerThreadCallback onEntry = nullptr, TProfilerThreadCallback onExit = nullptr)
		{
			CRY_ASSERT_MESSAGE(string(szProfilerName).find(' ') == string::npos, "Profiler name mustn't contain spaces");
			Detail::Get().emplace_back(SEntry{
				szProfilerName, szClArgument,
				[]() -> CCryProfilingSystemImpl* {return new TProfiler; },
				TProfiler::StartSectionStatic, TProfiler::RecordMarkerStatic,
				onEntry, onExit
			});
		}
	};

	const std::vector<SEntry>& Get();

	void ExecuteOnThreadEntryCallbacks();
	void ExecuteOnThreadExitCallbacks();
}}

// Registers a class as a profiler
// The profiler need to be a default constructible subclass of CCryProfilingSystemImpl.
// It needs to expose the static functions StartSectionStatic() and RecordMarkerStatic()
// which will be registered into gEnv->startProfilingSection and gEnv->recordProfilingMarker.
// className: typename of the class to be registered as profiler
// szProfilerName: name of the profiler, used in runtime switching (mustn't contain spaces)
// szClArgument: command line switch to select this profiler at startup
// There are two optional arguments. These can be functions conforming to Cry::ProfilerRegistry::TProfilerThreadCallback
// and will be called when threads are un/registered (the first on registration, the second on un-registration).
// These thread callbacks are called for all registered profilers (even inactive ones).
#ifdef ENABLE_PROFILING_CODE
#define REGISTER_PROFILER(className, szProfilerName, szClArgument, ...) \
	static Cry::ProfilerRegistry::SRegistrar<className> s_ ## className ## Registrar(szProfilerName, szClArgument, ## __VA_ARGS__) 
#else
#define REGISTER_PROFILER(className, szProfilerName, szClArgument, ...)
#endif
