// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>

class CAudioCVars
{
public:

	CAudioCVars();
	~CAudioCVars();

	void RegisterVariables();
	void UnregisterVariables();

	int   m_audioPrimaryPoolSize;
	int   m_fileCacheManagerSize;
	int   m_audioObjectPoolSize;
	int   m_audioEventPoolSize;
	int   m_audioStandaloneFilePoolSize;
	int   m_audioProxiesInitType;
	int   m_tickWithMainThread;

	float m_occlusionMaxDistance;
	float m_occlusionMaxSyncDistance;
	float m_occlusionHighDistance;
	float m_occlusionMediumDistance;
	float m_fullObstructionMaxDistance;
	float m_positionUpdateThreshold;
	float m_velocityTrackingThreshold;
	float m_occlusionRayLengthOffset;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	int    m_ignoreWindowFocus;
	int    m_drawAudioDebug;
	int    m_fileCacheManagerDebugFilter;
	int    m_audioLoggingOptions;
	int    m_showActiveAudioObjectsOnly;
	int    m_audioObjectsRayType;
	ICVar* m_pAudioTriggersDebugFilter;
	ICVar* m_pAudioObjectsDebugFilter;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

private:

	static void CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs);
	static void CmdStopTrigger(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetRtpc(IConsoleCmdArgs* pCmdArgs);
	static void CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs);

	PREVENT_OBJECT_COPY(CAudioCVars);
};

extern CAudioCVars g_audioCVars;
