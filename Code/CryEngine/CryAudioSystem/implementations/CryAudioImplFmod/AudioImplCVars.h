// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CAudioImplCVars final
{
public:

	CAudioImplCVars() = default;
	CAudioImplCVars(CAudioImplCVars const&) = delete;
	CAudioImplCVars(CAudioImplCVars&&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars const&) = delete;
	CAudioImplCVars& operator=(CAudioImplCVars&&) = delete;

	void             RegisterVariables();
	void             UnregisterVariables();

	int   m_maxChannels = 0;
	int   m_enableLiveUpdate = 0;
	int   m_enableSynchronousUpdate = 1;

	float m_lowpassMinCutoffFrequency = 10.0f;
	float m_distanceFactor = 1.0f;
	float m_dopplerScale = 1.0f;
	float m_rolloffScale = 1.0f;

#if CRY_PLATFORM_DURANGO
	int m_secondaryMemoryPoolSize = 0;
#endif  // CRY_PLATFORM_DURANGO

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};

extern CAudioImplCVars g_audioImplCVars;
}
}
}
