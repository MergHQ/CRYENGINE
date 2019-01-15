// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "System.h"
#include "Object.h"
#include "LoseFocusTrigger.h"
#include "GetFocusTrigger.h"
#include "MuteAllTrigger.h"
#include "UnmuteAllTrigger.h"
#include "PauseAllTrigger.h"
#include "ResumeAllTrigger.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "PreviewTrigger.h"
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

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
CObject* g_pObject = nullptr;
CLoseFocusTrigger g_loseFocusTrigger;
CGetFocusTrigger g_getFocusTrigger;
CMuteAllTrigger g_muteAllTrigger;
CUnmuteAllTrigger g_unmuteAllTrigger;
CPauseAllTrigger g_pauseAllTrigger;
CResumeAllTrigger g_resumeAllTrigger;

SImplInfo g_implInfo;
CryFixedStringT<MaxFilePathLength> g_configPath = "";

TriggerInstanceId g_triggerInstanceIdCounter = 1;

SPoolSizes g_poolSizes;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
CPreviewTrigger g_previewTrigger;
CObject g_previewObject(CTransformation::GetEmptyObject());
SPoolSizes g_debugPoolSizes;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
