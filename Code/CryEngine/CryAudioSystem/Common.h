// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
static constexpr ControlId LoseFocusTriggerId = StringToId(s_szLoseFocusTriggerName);
static constexpr ControlId GetFocusTriggerId = StringToId(s_szGetFocusTriggerName);
static constexpr ControlId MuteAllTriggerId = StringToId(s_szMuteAllTriggerName);
static constexpr ControlId UnmuteAllTriggerId = StringToId(s_szUnmuteAllTriggerName);
static constexpr ControlId PauseAllTriggerId = StringToId(s_szPauseAllTriggerName);
static constexpr ControlId ResumeAllTriggerId = StringToId(s_szResumeAllTriggerName);

namespace Impl
{
struct IImpl;
} // namespace Impl

class CSystem;
class CATLAudioObject;
class CLoseFocusTrigger;
class CGetFocusTrigger;
class CMuteAllTrigger;
class CUnmuteAllTrigger;
class CPauseAllTrigger;
class CResumeAllTrigger;
class CTrigger;
class CParameter;
class CATLSwitch;
class CATLPreloadRequest;
class CATLAudioEnvironment;
class CSetting;

enum class ESystemStates : EnumFlagsType
{
	None             = 0,
	ImplShuttingDown = BIT(0),
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	IsMuted          = BIT(1),
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ESystemStates);

using AudioTriggerLookup = std::map<ControlId, CTrigger const*>;
using AudioParameterLookup = std::map<ControlId, CParameter const*>;
using AudioSwitchLookup = std::map<ControlId, CATLSwitch const*>;
using AudioPreloadRequestLookup = std::map<PreloadRequestId, CATLPreloadRequest*>;
using AudioEnvironmentLookup = std::map<EnvironmentId, CATLAudioEnvironment const*>;
using SettingLookup = std::map<ControlId, CSetting const*>;

extern TriggerImplId g_uniqueConnectionId;
extern Impl::IImpl* g_pIImpl;
extern CSystem g_system;
extern ESystemStates g_systemStates;
extern AudioTriggerLookup g_triggers;
extern AudioParameterLookup g_parameters;
extern AudioSwitchLookup g_switches;
extern AudioPreloadRequestLookup g_preloadRequests;
extern AudioEnvironmentLookup g_environments;
extern SettingLookup g_settings;
extern CATLAudioObject* g_pObject;
extern CLoseFocusTrigger g_loseFocusTrigger;
extern CGetFocusTrigger g_getFocusTrigger;
extern CMuteAllTrigger g_muteAllTrigger;
extern CUnmuteAllTrigger g_unmuteAllTrigger;
extern CPauseAllTrigger g_pauseAllTrigger;
extern CResumeAllTrigger g_resumeAllTrigger;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
static constexpr char const* s_szPreviewTriggerName = "preview_trigger";
static constexpr ControlId PreviewTriggerId = StringToId(s_szPreviewTriggerName);

class CPreviewTrigger;
extern CPreviewTrigger g_previewTrigger;
extern CATLAudioObject g_previewObject;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
