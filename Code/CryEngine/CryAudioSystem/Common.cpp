// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "AudioSystem.h"
#include "ATL.h"

namespace CryAudio
{
Impl::IImpl* g_pIImpl = nullptr;
CSystem g_system;
CAudioTranslationLayer g_atl;
AudioTriggerLookup g_triggers;
AudioParameterLookup g_parameters;
AudioSwitchLookup g_switches;
AudioPreloadRequestLookup g_preloadRequests;
AudioEnvironmentLookup g_environments;
CATLAudioObject* g_pObject = nullptr;
CAbsoluteVelocityParameter* g_pAbsoluteVelocityParameter = nullptr;
CRelativeVelocityParameter* g_pRelativeVelocityParameter = nullptr;
CLoseFocusTrigger* g_pLoseFocusTrigger = nullptr;
CGetFocusTrigger* g_pGetFocusTrigger = nullptr;
CMuteAllTrigger* g_pMuteAllTrigger = nullptr;
CUnmuteAllTrigger* g_pUnmuteAllTrigger = nullptr;
CPauseAllTrigger* g_pPauseAllTrigger = nullptr;
CResumeAllTrigger* g_pResumeAllTrigger = nullptr;
} // namespace CryAudio
