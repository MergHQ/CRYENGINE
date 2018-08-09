// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CImpl;

class CCVars final
{
public:

	CCVars() = default;
	CCVars(CCVars const&) = delete;
	CCVars(CCVars&&) = delete;
	CCVars& operator=(CCVars const&) = delete;
	CCVars& operator=(CCVars&&) = delete;

	void    RegisterVariables();
	void    UnregisterVariables();

	int   m_maxVirtualVoices = 0;
	int   m_maxVoiceLimitGroups = 0;
	int   m_maxCategories = 0;
	int   m_categoriesPerPlayback = 0;
	int   m_maxTracks = 0;
	int   m_maxTrackItems = 0;
	int   m_maxFaders = 0;
	int   m_numVoices = 0;
	int   m_maxChannels = 0;
	int   m_maxSamplingRate = 0;
	int   m_numBuses = 0;
	int   m_outputChannels = 0;
	int   m_outputSamplingRate = 0;
	int   m_maxStreams = 0;
	int   m_maxBps = 0;
	int   m_maxFiles = 0;
	int   m_voiceAllocationMethod = 0;

	float m_maxPitch = 0;
};

extern CCVars g_cvars;
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
