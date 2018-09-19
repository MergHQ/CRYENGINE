// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"
#include "AudioSystem.h"
#include "ATLEntities.h"
#include "ATLAudioObject.h"

namespace CryAudio
{
TriggerImplId g_uniqueConnectionId = 0;
Impl::IImpl* g_pIImpl = nullptr;
CSystem g_system;
ESystemStates g_systemStates = ESystemStates::None;
AudioTriggerLookup g_triggers;
AudioParameterLookup g_parameters;
AudioSwitchLookup g_switches;
AudioPreloadRequestLookup g_preloadRequests;
AudioEnvironmentLookup g_environments;
SettingLookup g_settings;
CATLAudioObject* g_pObject = nullptr;
CLoseFocusTrigger g_loseFocusTrigger;
CGetFocusTrigger g_getFocusTrigger;
CMuteAllTrigger g_muteAllTrigger;
CUnmuteAllTrigger g_unmuteAllTrigger;
CPauseAllTrigger g_pauseAllTrigger;
CResumeAllTrigger g_resumeAllTrigger;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
CPreviewTrigger g_previewTrigger;
CATLAudioObject g_previewObject(CObjectTransformation::GetEmptyObject());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
