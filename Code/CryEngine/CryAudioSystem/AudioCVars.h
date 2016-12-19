// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

namespace CryAudio
{
class CAudioCVars final
{
public:

	CAudioCVars() = default;
	CAudioCVars(CAudioCVars const&) = delete;
	CAudioCVars(CAudioCVars&&) = delete;
	CAudioCVars& operator=(CAudioCVars const&) = delete;
	CAudioCVars& operator=(CAudioCVars&&) = delete;

	void         RegisterVariables();
	void         UnregisterVariables();

	int   m_fileCacheManagerSize = 0;
	int   m_audioObjectPoolSize = 0;
	int   m_audioEventPoolSize = 0;
	int   m_audioStandaloneFilePoolSize = 0;
	int   m_audioProxiesInitType = 0;
	int   m_tickWithMainThread = 0;

	float m_occlusionMaxDistance = 0.0f;
	float m_occlusionMaxSyncDistance = 0.0f;
	float m_occlusionHighDistance = 0.0f;
	float m_occlusionMediumDistance = 0.0f;
	float m_fullObstructionMaxDistance = 0.0f;
	float m_positionUpdateThresholdMultiplier = 0.02f;
	float m_velocityTrackingThreshold = 0.0f;
	float m_occlusionRayLengthOffset = 0.0f;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	int    m_ignoreWindowFocus = 0;
	int    m_drawAudioDebug = 0;
	int    m_fileCacheManagerDebugFilter = 0;
	int    m_audioLoggingOptions = 0;
	int    m_showActiveAudioObjectsOnly = 0;
	int    m_audioObjectsRayType = 0;
	ICVar* m_pAudioTriggersDebugFilter = nullptr;
	ICVar* m_pAudioObjectsDebugFilter = nullptr;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	static void CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs);
	static void CmdStopTrigger(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetRtpc(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs);
};
} // namespace CryAudio

extern CryAudio::CAudioCVars g_audioCVars;
