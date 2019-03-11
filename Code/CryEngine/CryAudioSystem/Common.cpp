// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "System.h"
#include "Object.h"
#include "GlobalObject.h"
#include "LoseFocusTrigger.h"
#include "GetFocusTrigger.h"
#include "MuteAllTrigger.h"
#include "UnmuteAllTrigger.h"
#include "PauseAllTrigger.h"
#include "ResumeAllTrigger.h"

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
	#include "PreviewTrigger.h"
#endif // CRY_AUDIO_USE_DEBUG_CODE

namespace CryAudio
{
Impl::IImpl* g_pIImpl = nullptr;
CSystem g_system;
ESystemStates g_systemStates = ESystemStates::None;
TriggerLookup g_triggers;
ParameterLookup g_parameters;
SwitchLookup g_switches;
PreloadRequestLookup g_preloadRequests;
EnvironmentLookup g_environments;
SettingLookup g_settings;
TriggerInstanceIdLookup g_triggerInstanceIdToObject;
TriggerInstanceIdLookupGlobal g_triggerInstanceIdToGlobalObject;
ContextLookup g_registeredContexts;

CLoseFocusTrigger g_loseFocusTrigger;
CGetFocusTrigger g_getFocusTrigger;
CMuteAllTrigger g_muteAllTrigger;
CUnmuteAllTrigger g_unmuteAllTrigger;
CPauseAllTrigger g_pauseAllTrigger;
CResumeAllTrigger g_resumeAllTrigger;
Objects g_activeObjects;

SImplInfo g_implInfo;
CryFixedStringT<MaxFilePathLength> g_configPath = "";

TriggerInstanceId g_triggerInstanceIdCounter = 1;

SPoolSizes g_poolSizes;

#if defined(CRY_AUDIO_USE_DEBUG_CODE)
Objects g_constructedObjects;
CGlobalObject g_object("Global Object");
CGlobalObject g_previewObject("Preview Object");
CPreviewTrigger g_previewTrigger;
SPoolSizes g_debugPoolSizes;
ContextLookup g_activeContexts;
#else
CGlobalObject g_object;
#endif // CRY_AUDIO_USE_DEBUG_CODE
}      // namespace CryAudio
