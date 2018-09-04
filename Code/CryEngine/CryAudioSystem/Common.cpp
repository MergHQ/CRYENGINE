// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "AudioSystem.h"
#include "ATLEntities.h"

namespace CryAudio
{
Impl::IImpl* g_pIImpl = nullptr;
CSystem g_system;
AudioTriggerLookup g_triggers;
AudioParameterLookup g_parameters;
AudioSwitchLookup g_switches;
AudioPreloadRequestLookup g_preloadRequests;
AudioEnvironmentLookup g_environments;
CATLAudioObject* g_pObject = nullptr;
CLoseFocusTrigger g_loseFocusTrigger;
CGetFocusTrigger g_getFocusTrigger;
CMuteAllTrigger g_muteAllTrigger;
CUnmuteAllTrigger g_unmuteAllTrigger;
CPauseAllTrigger g_pauseAllTrigger;
CResumeAllTrigger g_resumeAllTrigger;
} // namespace CryAudio
