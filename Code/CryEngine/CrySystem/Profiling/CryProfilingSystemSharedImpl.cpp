// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryProfilingSystemSharedImpl.h"
#include <CryCore/CryCrc32.h>

CCryProfilingSystemImpl::CCryProfilingSystemImpl() = default;

bool CCryProfilingSystemImpl::OnInputEvent(const SInputEvent& event)
{
	if (event.deviceType == eIDT_Keyboard && event.keyId == eKI_ScrollLock)
	{
		if (event.state == eIS_Pressed)
		{
			PauseRecording(!IsPaused());
		}
	}
	return false;
}

void CCryProfilingSystemImpl::OnSliceAndSleep()
{
	EndFrame();
	StartFrame();
}

const char* CCryProfilingSystemImpl::GetModuleName(EProfiledSubsystem subSystem) const
{
	switch (subSystem)
	{
	case PROFILE_RENDERER:
		return "Renderer";
	case PROFILE_3DENGINE:
		return "3DEngine";
	case PROFILE_PARTICLE:
		return "Particles";
	case PROFILE_AI:
		return "AI";
	case PROFILE_ANIMATION:
		return "Animation";
	case PROFILE_MOVIE:
		return "Movie";
	case PROFILE_ENTITY:
		return "Entities";
	case PROFILE_FONT:
		return "Font";
	case PROFILE_NETWORK:
		return "Network";
	case PROFILE_PHYSICS:
		return "Physics";
	case PROFILE_SCRIPT:
		return "Script";
	case PROFILE_SCRIPT_CFUNC:
		return "Script_CFunc";
	case PROFILE_AUDIO:
		return "Audio";
	case PROFILE_EDITOR:
		return "Editor";
	case PROFILE_SYSTEM:
		return "System";
	case PROFILE_ACTION:
		return "Action";
	case PROFILE_GAME:
		return "Game";
	case PROFILE_INPUT:
		return "Input";
	case PROFILE_LOADING_ONLY:
		return "Loading";
	default:
		CRY_ASSERT(false);
		return "<Unknown Id>";
	}
}

const char* CCryProfilingSystemImpl::GetModuleName(const SProfilingSection* pSection) const
{
	return GetModuleName(pSection->pDescription->subsystem);
}

uint32 CCryProfilingSystemImpl::GenerateColorBasedOnName(const char* name)
{
	uint32 hash = CCrc32::Compute(name);
	hash |= 0xFF000000; // force alpha to opaque
	return hash;
}

void CCryProfilingSystemImpl::DescriptionCreated(SProfilingDescription* pDesc)
{
	pDesc->color_argb = GenerateColorBasedOnName(pDesc->szEventname);
	m_descriptions.push_back(pDesc);
}

void CCryProfilingSystemImpl::DescriptionDestroyed(SProfilingDescription* pDesc)
{
	m_descriptions.try_remove(pDesc);
}

std::vector<SProfilingDescription*> CCryProfilingSystemImpl::ReleaseDescriptions()
{
	std::vector<SProfilingDescription*> result;
	m_descriptions.swap(result);
	for (SProfilingDescription* pDesc : result)
	{
		DescriptionDestroyed(pDesc);
	}
	return result;
}

namespace Cry {
namespace ProfilerRegistry
{
	const std::vector<SEntry>& Get()
	{
		return Detail::Get();
	}

	void ExecuteOnThreadEntryCallbacks()
	{
#ifdef ENABLE_PROFILING_CODE
		for (const Cry::ProfilerRegistry::SEntry& profiler : Cry::ProfilerRegistry::Get())
		{
			if (profiler.onThreadEntry)
				profiler.onThreadEntry();
		}
#endif
	}

	void ExecuteOnThreadExitCallbacks()
	{
#ifdef ENABLE_PROFILING_CODE
		for (const Cry::ProfilerRegistry::SEntry& profiler : Cry::ProfilerRegistry::Get())
		{
			if (profiler.onThreadExit)
				profiler.onThreadExit();
		}
#endif
	}

	namespace Detail
	{
		// wrapped in a function to avoid static initialization order problems
		std::vector<SEntry>& Get()
		{
			static std::vector<SEntry> s_profilerRegistry;
			return s_profilerRegistry;
		}
	}
}}
