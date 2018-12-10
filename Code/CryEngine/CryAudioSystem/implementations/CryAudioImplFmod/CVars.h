// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CCVars final
{
public:

	CCVars(CCVars const&) = delete;
	CCVars(CCVars&&) = delete;
	CCVars& operator=(CCVars const&) = delete;
	CCVars& operator=(CCVars&&) = delete;

	CCVars() = default;

	void RegisterVariables();
	void UnregisterVariables();

	int   m_maxChannels = 0;
	int   m_enableSynchronousUpdate = 1;

	float m_velocityTrackingThreshold = 0.0f;
	float m_positionUpdateThresholdMultiplier = 0.02f;
	float m_lowpassMinCutoffFrequency = 10.0f;
	float m_distanceFactor = 1.0f;
	float m_dopplerScale = 1.0f;
	float m_rolloffScale = 1.0f;

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	int m_enableLiveUpdate = 0;
#endif  // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
};

extern CCVars g_cvars;
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
