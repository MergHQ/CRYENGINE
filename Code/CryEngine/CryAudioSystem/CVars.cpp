// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CVars.h"
#include "Common.h"
#include "System.h"
#include <CrySystem/IConsole.h>

#if defined(CRY_AUDIO_USE_OCCLUSION)
	#include "PropagationProcessor.h"
#endif // CRY_AUDIO_USE_OCCLUSION

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "Common/Logger.h"
	#include <CryGame/IGameFramework.h>
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
#if defined(CRY_AUDIO_USE_OCCLUSION)
//////////////////////////////////////////////////////////////////////////
void OnOcclusionRayTypesChanged(ICVar* const pCvar)
{
	CPropagationProcessor::UpdateOcclusionRayFlags();
}

//////////////////////////////////////////////////////////////////////////
void OnOcclusionPlaneSizeChanged(ICVar* const pCvar)
{
	CPropagationProcessor::UpdateOcclusionPlanes();
}
#endif // CRY_AUDIO_USE_OCCLUSION

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CmdExecuteTrigger(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 2)
	{
		ControlId const triggerId = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->ExecuteTrigger(triggerId);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_ExecuteTrigger [TriggerName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdStopTrigger(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 1)
	{
		gEnv->pAudioSystem->StopTrigger(InvalidControlId);
	}
	else if (numArgs == 2)
	{
		ControlId const triggerId = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->StopTrigger(triggerId);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_StopTrigger [TriggerName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdSetParameter(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 3)
	{
		ControlId const parameterId = StringToId(pCmdArgs->GetArg(1));
		double const value = atof(pCmdArgs->GetArg(2));
		gEnv->pAudioSystem->SetParameter(parameterId, static_cast<float>(value));
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_SetParameter [ParameterName] [ParameterValue]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdSetParameterGlobally(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 3)
	{
		ControlId const parameterId = StringToId(pCmdArgs->GetArg(1));
		double const value = atof(pCmdArgs->GetArg(2));
		gEnv->pAudioSystem->SetParameterGlobally(parameterId, static_cast<float>(value));
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_SetParameterGlobally [ParameterName] [ParameterValue]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdSetSwitchState(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 3)
	{
		ControlId const switchId = StringToId(pCmdArgs->GetArg(1));
		SwitchStateId const switchStateId = StringToId(pCmdArgs->GetArg(2));
		gEnv->pAudioSystem->SetSwitchState(switchId, switchStateId);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_SetSwitchState [SwitchName] [SwitchStateName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdSetSwitchStateGlobally(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 3)
	{
		ControlId const switchId = StringToId(pCmdArgs->GetArg(1));
		SwitchStateId const switchStateId = StringToId(pCmdArgs->GetArg(2));
		gEnv->pAudioSystem->SetSwitchStateGlobally(switchId, switchStateId);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_SetSwitchStateGlobally [SwitchName] [SwitchStateName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdLoadRequest(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 2)
	{
		ControlId const id = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->PreloadSingleRequest(id, false);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_LoadRequest [PreloadRequestName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdUnloadRequest(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 2)
	{
		ControlId const id = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->UnloadSingleRequest(id);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_UnloadRequest [PreloadRequestName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdLoadSetting(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 2)
	{
		ControlId const id = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->LoadSetting(id);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_LoadSetting [SettingName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdUnloadSetting(IConsoleCmdArgs* pCmdArgs)
{
	int const numArgs = pCmdArgs->GetArgCount();

	if (numArgs == 2)
	{
		ControlId const id = StringToId(pCmdArgs->GetArg(1));
		gEnv->pAudioSystem->UnloadSetting(id);
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Usage: s_UnloadSetting [SettingName]");
	}
}

//////////////////////////////////////////////////////////////////////////
void CmdResetRequestCount(IConsoleCmdArgs* pCmdArgs)
{
	g_system.ResetRequestCount();
}

//////////////////////////////////////////////////////////////////////////
void CmdRefresh(IConsoleCmdArgs* pCmdArgs)
{
	GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_AUDIO_REFRESH, reinterpret_cast<UINT_PTR>(gEnv->pGameFramework->GetLevelName()), 0);
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CCVars::RegisterVariables()
{
	m_triggerInstancePoolSize = 512;

#if CRY_PLATFORM_WINDOWS
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on PC
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_DURANGO
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on XboxOne
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_ORBIS
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on PS4
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_MAC
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on Mac
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_LINUX
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on Linux
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_IOS
	m_fileCacheManagerSize = 384 << 10;      // 384 MiB on iOS
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#elif CRY_PLATFORM_ANDROID
	m_fileCacheManagerSize = 72 << 10;      // 72 MiB
	m_objectPoolSize = 256;
	#if defined(CRY_AUDIO_USE_OCCLUSION)
	m_occlusionMaxSyncDistance = 10.0f;
	m_occlusionHighDistance = 10.0f;
	m_occlusionMediumDistance = 80.0f;
	#endif // CRY_AUDIO_USE_OCCLUSION
#else
	#error "Undefined platform."
#endif

	REGISTER_CVAR2("s_FileCacheManagerSize", &m_fileCacheManagerSize, m_fileCacheManagerSize, VF_REQUIRE_APP_RESTART,
	               "Sets the size in KiB the AFCM will allocate on the heap.\n"
	               "Usage: s_FileCacheManagerSize [0/...]\n"
	               "Default PC: 393216 (384 MiB), XboxOne: 393216 (384 MiB), PS4: 393216 (384 MiB), Mac: 393216 (384 MiB), Linux: 393216 (384 MiB), iOS: 2048 (2 MiB), Android: 73728 (72 MiB)\n");

	REGISTER_CVAR2("s_AudioObjectPoolSize", &m_objectPoolSize, m_objectPoolSize, VF_REQUIRE_APP_RESTART,
	               "Sets the number of preallocated audio objects and corresponding audio proxies.\n"
	               "Usage: s_AudioObjectPoolSize [0/...]\n"
	               "Default PC: 256, XboxOne: 256, PS4: 256, Mac: 256, Linux: 256, iOS: 256, Android: 256\n");

	REGISTER_CVAR2("s_TriggerInstancePoolSize", &m_triggerInstancePoolSize, m_triggerInstancePoolSize, VF_REQUIRE_APP_RESTART,
	               "Sets the number of preallocated trigger instances.\n"
	               "Usage: s_TriggerInstancePoolSize [0/...]\n"
	               "Default: 512\n");

	REGISTER_CVAR2("s_IgnoreWindowFocus", &m_ignoreWindowFocus, 0, VF_NULL,
	               "If set to 1, the audio system will not execute the \"lose_focus\" and \"get_focus\" triggers when the application window focus changes.\n"
	               "Usage: s_IgnoreWindowFocus [0/1]\n"
	               "Default: 0 (off)\n");

#if defined(CRY_AUDIO_USE_OCCLUSION)
	REGISTER_CVAR2("s_OcclusionMaxDistance", &m_occlusionMaxDistance, m_occlusionMaxDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Occlusion is not calculated for audio objects, whose distance to the listener is greater than this value. Setting this value to 0 disables obstruction/occlusion calculations.\n"
	               "Usage: s_OcclusionMaxDistance [0/...]\n"
	               "Default: 500 m\n");

	REGISTER_CVAR2("s_OcclusionMinDistance", &m_occlusionMinDistance, m_occlusionMinDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Occlusion is not calculated for audio objects, whose distance to the listener is smaller than this value.\n"
	               "Usage: s_OcclusionMinDistance [0/...]\n"
	               "Default: 0.1 m\n");

	REGISTER_CVAR2("s_OcclusionMaxSyncDistance", &m_occlusionMaxSyncDistance, m_occlusionMaxSyncDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Physics rays are processed synchronously for the sounds that are closer to the listener than this value, and asynchronously for the rest (possible performance optimization).\n"
	               "Usage: s_OcclusionMaxSyncDistance [0/...]\n"
	               "Default: 10 m\n");

	REGISTER_CVAR2("s_OcclusionHighDistance", &m_occlusionHighDistance, m_occlusionHighDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Within this distance occlusion calculation uses the most sample points for highest granularity.\n"
	               "Usage: s_OcclusionHighDistance [0/...]\n"
	               "Default: 10 m\n");

	REGISTER_CVAR2("s_OcclusionMediumDistance", &m_occlusionMediumDistance, m_occlusionMediumDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Between end of high and this distance occlusion calculation uses a medium amount of sample points for medium granularity.\n"
	               "Usage: s_OcclusionMediumDistance [0/...]\n"
	               "Default: 80 m\n");

	REGISTER_CVAR2_CB("s_OcclusionListenerPlaneSize", &m_occlusionListenerPlaneSize, 1.0f, VF_CHEAT | VF_CHEAT_NOCHECK,
	                  "Sets the size of the plane at listener position against which occlusion is calculated.\n"
	                  "Usage: s_OcclusionListenerPlaneSize [0/...]\n"
	                  "Default: 1.0 (100 cm)\n",
	                  OnOcclusionPlaneSizeChanged);

	REGISTER_CVAR2("s_OcclusionAccumulate", &m_occlusionAccumulate, m_occlusionAccumulate, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Sets whether occlusion values encountered by a ray cast will be accumulated or only the highest value will be used.\n"
	               "Usage: s_OcclusionAccumulate [0/1] (off/on)\n"
	               "Default PC: 1, XboxOne: 1, PS4: 1, Mac: 1, Linux: 1, iOS: 1, Android: 1\n");

	REGISTER_CVAR2_CB("s_OcclusionCollisionTypes", &m_occlusionCollisionTypes, AlphaBits64("abcd"), VF_CHEAT | VF_BITFIELD,
	                  "Sets which types of ray casting collision hits are used to calculate occlusion.\n"
	                  "Usage: s_OcclusionCollisionTypes [0ab...] (flags can be combined)\n"
	                  "Default: abcd\n"
	                  "0: No collisions.\n"
	                  "a: Collisions with static objects.\n"
	                  "b: Collisions with rigid entities.\n"
	                  "c: Collisions with water.\n"
	                  "d: Collisions with terrain.\n",
	                  OnOcclusionRayTypesChanged);

	REGISTER_CVAR2("s_OcclusionSetFullOnMaxHits", &m_occlusionSetFullOnMaxHits, m_occlusionSetFullOnMaxHits, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "If set to 1, the occlusion value will be set to 1 (max) when a ray cast reaches its max hit limit.\n"
	               "Usage: s_OcclusionSetFullOnMaxHits [0/1] (off/on)\n"
	               "Default: 0 (off)\n");

	REGISTER_CVAR2("s_OcclusionInitialRayCastMode", &m_occlusionInitialRayCastMode, m_occlusionInitialRayCastMode, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Sets the ray cast mode of the initial occlusion check when a trigger gets executed.\n"
	               "Usage: s_OcclusionInitialRayCastMode [0/...]\n"
	               "Default: 1\n"
	               "0: No initial occlusion check.\n"
	               "1: Initial occlusion check uses 1 ray from the center of the listener occlusion plane.\n"
	               "2: Initial occlusion check uses 5 rays. The center ray and one at each corner of the listener occlusion plane.\n");

	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	REGISTER_CVAR2("s_OcclusionGlobalType", &m_occlusionGlobalType, m_occlusionGlobalType, VF_DEV_ONLY,
	               "Can override audio objects' obstruction/occlusion ray type on a global scale.\n"
	               "If set it determines whether audio objects use no, adaptive, low, medium or high granularity for rays.\n"
	               "This is a performance type cvar and can be used to turn audio ray casting globally off\n"
	               "or force it on every audio objects to a given mode.\n"
	               "0: Audio object specific ray casting.\n"
	               "1: All audio objects ignore ray casting.\n"
	               "2: All audio objects use adaptive ray casting.\n"
	               "3: All audio objects use low ray casting.\n"
	               "4: All audio objects use medium ray casting.\n"
	               "5: All audio objects use high ray casting.\n"
	               "Usage: s_OcclusionGlobalType [0/1/2/3/4/5]\n"
	               "Default PC: 0, XboxOne: 0, PS4: 0, Mac: 0, Linux: 0, iOS: 0, Android: 0\n");
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
#endif   // CRY_AUDIO_USE_OCCLUSION

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	REGISTER_CVAR2("s_DebugDistance", &m_debugDistance, m_debugDistance, VF_CHEAT | VF_CHEAT_NOCHECK,
	               "Limits drawing of audio object debug info to the specified distance around the active listeners. Setting this cvar to 0 disables the limiting.\n"
	               "Usage: s_DebugDistance [0/...]\n"
	               "Default: 0 m (infinite)\n");

	REGISTER_CVAR2("s_LoggingOptions", &m_loggingOptions, AlphaBits64("abc"), VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
	               "Toggles the logging of audio related messages.\n"
	               "Usage: s_LoggingOptions [0ab...] (flags can be combined)\n"
	               "Default: abc\n"
	               "0: Logging disabled\n"
	               "a: Errors\n"
	               "b: Warnings\n"
	               "c: Comments\n");

	REGISTER_CVAR2("s_DrawAudioDebug", &m_drawDebug, 0, VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
	               "Draws AudioTranslationLayer related debug data to the screen.\n"
	               "Usage: s_DrawAudioDebug [0ab...] (flags can be combined)\n"
	               "0: No audio debug info on the screen.\n"
	               "a: Draw spheres around active audio objects.\n"
	               "b: Draw text labels for active audio objects.\n"
	               "c: Draw trigger names for active audio objects.\n"
	               "d: Draw current states for active audio objects.\n"
	               "e: Draw Parameter values for active audio objects.\n"
	               "f: Draw Environment amounts for active audio objects.\n"
	               "g: Draw distance to listener for active audio objects.\n"
	               "h: Draw occlusion ray labels.\n"
	               "i: Draw occlusion rays.\n"
	               "j: Draw spheres with occlusion ray offset radius around active audio objects.\n"
	               "k: Draw listener occlusion plane.\n"
	               "l: Draw global object info.\n"
	               "m: Draw middleware specific info for active audio objects.\n"
	               "q: Hide audio system memory info.\n"
	               "r: Apply filter also to inactive object debug info.\n"
	               "s: Draw detailed memory pool debug info.\n"
	               "v: List implementation specific info.\n"
	               "w: List active audio objects.\n"
	               "x: Draw FileCache Manager debug info.\n"
	               "y: Draw Request debug info.\n"
	               );

	REGISTER_CVAR2("s_FileCacheManagerDebugFilter", &m_fileCacheManagerDebugFilter, 0, VF_CHEAT | VF_CHEAT_NOCHECK | VF_BITFIELD,
	               "Allows for filtered display of the different AFCM entries such as Globals, Level Specifics, Game Hints and so on.\n"
	               "Usage: s_FileCacheManagerDebugFilter [0ab...] (flags can be combined)\n"
	               "Default: 0 (all)\n"
	               "a: Globals\n"
	               "b: Level Specifics\n"
	               "c: Game Hints\n");

	REGISTER_CVAR2("s_HideInactiveAudioObjects", &m_hideInactiveObjects, 1, VF_DEV_ONLY,
	               "When drawing audio object names on the screen this cvar can be used to choose between all registered audio objects or only those that reference active audio triggers.\n"
	               "Usage: s_HideInactiveAudioObjects [0/1]\n"
	               "Default: 1 (active only)\n");

	m_pDebugFilter = REGISTER_STRING("s_DebugFilter", "", 0,
	                                 "Allows for filtered display of audio debug info by a search string.\n"
	                                 "Usage: s_DebugFilter spaceship\n"
	                                 "Default: " " (all)\n");

	REGISTER_COMMAND("s_ExecuteTrigger", CmdExecuteTrigger, VF_CHEAT,
	                 "Execute an Audio Trigger.\n"
	                 "The argument is the name of the trigger to be executed on the Global Object.\n"
	                 "Usage: s_ExecuteTrigger MuteDialog\n");

	REGISTER_COMMAND("s_StopTrigger", CmdStopTrigger, VF_CHEAT,
	                 "Execute an Audio Trigger.\n"
	                 "The argument is the name of the trigger to be stopped on the Global Object.\n"
	                 "If no argument ois provided, all playing triggers on the Global Object get stopped.\n"
	                 "Usage: s_StopTrigger MuteDialog\n");

	REGISTER_COMMAND("s_SetParameter", CmdSetParameter, VF_CHEAT,
	                 "Set an Audio Parameter value.\n"
	                 "The first argument is the name of the parameter to be set, the second argument is the float value to be set."
	                 "The parameter is set on the Global Object.\n"
	                 "Usage: s_SetParameter volume_music 1.0\n");

	REGISTER_COMMAND("s_SetParameterGlobally", CmdSetParameterGlobally, VF_CHEAT,
	                 "Set an Audio Parameter value.\n"
	                 "The first argument is the name of the parameter to be set, the second argument is the float value to be set."
	                 "The parameter is set on all constructed objects.\n"
	                 "Usage: s_SetParameterGlobally volume_music 1.0\n");

	REGISTER_COMMAND("s_SetSwitchState", CmdSetSwitchState, VF_CHEAT,
	                 "Set an Audio Switch to a provided State.\n"
	                 "The first argument is the name of the switch to, the second argument is the name of the state to be set."
	                 "The switch state is set on the Global Object.\n"
	                 "Usage: s_SetSwitchState weather rain\n");

	REGISTER_COMMAND("s_SetSwitchStateGlobally", CmdSetSwitchStateGlobally, VF_CHEAT,
	                 "Set an Audio Switch to a provided State.\n"
	                 "The first argument is the name of the switch to, the second argument is the name of the state to be set."
	                 "The switch state is set on all constructed objects.\n"
	                 "Usage: s_SetSwitchStateGlobally weather rain\n");

	REGISTER_COMMAND("s_LoadRequest", CmdLoadRequest, VF_CHEAT,
	                 "Loads a preload request. The preload request has to be non-autoloaded.\n"
	                 "The argument is the name of the preload request to load.\n"
	                 "Usage: s_LoadRequest VehiclePreload\n");

	REGISTER_COMMAND("s_UnloadRequest", CmdUnloadRequest, VF_CHEAT,
	                 "Unloads a preload request. The preload request has to be non-autoloaded.\n"
	                 "The argument is the name of the preload request to load.\n"
	                 "Usage: s_UnloadRequest VehiclePreload\n");

	REGISTER_COMMAND("s_LoadSetting", CmdLoadSetting, VF_CHEAT,
	                 "Loads a setting.\n"
	                 "The argument is the name of the setting to load.\n"
	                 "Usage: s_LoadSetting main_menu\n");

	REGISTER_COMMAND("s_UnloadSetting", CmdUnloadSetting, VF_CHEAT,
	                 "Unloads a setting.\n"
	                 "The argument is the name of the setting to unload.\n"
	                 "Usage: s_UnloadSetting main_menu\n");

	REGISTER_COMMAND("s_ResetRequestCount", CmdResetRequestCount, VF_CHEAT,
	                 "Resets the request counts shown in s_DrawAudioDebug y.\n"
	                 "Usage: s_resetRequestCount\n");

	REGISTER_COMMAND("s_Refresh", CmdRefresh, VF_CHEAT,
	                 "Refreshes the audio system.\n"
	                 "Usage: s_Refresh\n");
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CCVars::UnregisterVariables()
{
	IConsole* const pConsole = gEnv->pConsole;

	if (pConsole != nullptr)
	{
		pConsole->UnregisterVariable("s_FileCacheManagerSize");
		pConsole->UnregisterVariable("s_AudioObjectPoolSize");
		pConsole->UnregisterVariable("s_TriggerInstancePoolSize");
		pConsole->UnregisterVariable("s_IgnoreWindowFocus");

#if defined(CRY_AUDIO_USE_OCCLUSION)
		pConsole->UnregisterVariable("s_OcclusionMaxDistance");
		pConsole->UnregisterVariable("s_OcclusionMinDistance");
		pConsole->UnregisterVariable("s_OcclusionMaxSyncDistance");
		pConsole->UnregisterVariable("s_OcclusionHighDistance");
		pConsole->UnregisterVariable("s_OcclusionMediumDistance");
		pConsole->UnregisterVariable("s_OcclusionListenerPlaneSize");
		pConsole->UnregisterVariable("s_OcclusionAccumulate");
		pConsole->UnregisterVariable("s_OcclusionCollisionTypes");
		pConsole->UnregisterVariable("s_OcclusionSetFullOnMaxHits");
		pConsole->UnregisterVariable("s_OcclusionInitialRayCastMode");
	#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		pConsole->UnregisterVariable("s_OcclusionGlobalType");
	#endif // CRY_AUDIO_USE_PRODUCTION_CODE
#endif   // CRY_AUDIO_USE_OCCLUSION

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
		pConsole->UnregisterVariable("s_DebugDistance");
		pConsole->UnregisterVariable("s_LoggingOptions");
		pConsole->UnregisterVariable("s_DrawAudioDebug");
		pConsole->UnregisterVariable("s_FileCacheManagerDebugFilter");
		pConsole->UnregisterVariable("s_HideInactiveAudioObjects");

		pConsole->UnregisterVariable("s_DebugFilter");
		pConsole->UnregisterVariable("s_ExecuteTrigger");
		pConsole->UnregisterVariable("s_StopTrigger");
		pConsole->UnregisterVariable("s_SetParameter");
		pConsole->UnregisterVariable("s_SetParameterGlobally");
		pConsole->UnregisterVariable("s_SetSwitchState");
		pConsole->UnregisterVariable("s_SetSwitchStateGlobally");
		pConsole->UnregisterVariable("s_LoadRequest");
		pConsole->UnregisterVariable("s_UnloadRequest");
		pConsole->UnregisterVariable("s_LoadSetting");
		pConsole->UnregisterVariable("s_UnloadSetting");
		pConsole->UnregisterVariable("s_ResetRequestCount");
		pConsole->UnregisterVariable("s_Refresh");
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
	}
}
}      // namespace CryAudio
