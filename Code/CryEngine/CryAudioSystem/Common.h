// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
static constexpr ControlId RelativeVelocityParameterId = StringToId(s_szRelativeVelocityParameterName);
static constexpr ControlId AbsoluteVelocityParameterId = StringToId(s_szAbsoluteVelocityParameterName);
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
class CAudioTranslationLayer;
class CATLAudioObject;
class CAbsoluteVelocityParameter;
class CRelativeVelocityParameter;
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

using AudioTriggerLookup = std::map<ControlId, CTrigger const*>;
using AudioParameterLookup = std::map<ControlId, CParameter const*>;
using AudioSwitchLookup = std::map<ControlId, CATLSwitch const*>;
using AudioPreloadRequestLookup = std::map<PreloadRequestId, CATLPreloadRequest*>;
using AudioEnvironmentLookup = std::map<EnvironmentId, CATLAudioEnvironment const*>;

extern Impl::IImpl* g_pIImpl;
extern CSystem g_system;
extern CAudioTranslationLayer g_atl;
extern AudioTriggerLookup g_triggers;
extern AudioParameterLookup g_parameters;
extern AudioSwitchLookup g_switches;
extern AudioPreloadRequestLookup g_preloadRequests;
extern AudioEnvironmentLookup g_environments;
extern CATLAudioObject* g_pObject;
extern CAbsoluteVelocityParameter* g_pAbsoluteVelocityParameter;
extern CRelativeVelocityParameter* g_pRelativeVelocityParameter;
extern CLoseFocusTrigger* g_pLoseFocusTrigger;
extern CGetFocusTrigger* g_pGetFocusTrigger;
extern CMuteAllTrigger* g_pMuteAllTrigger;
extern CUnmuteAllTrigger* g_pUnmuteAllTrigger;
extern CPauseAllTrigger* g_pPauseAllTrigger;
extern CResumeAllTrigger* g_pResumeAllTrigger;
} // namespace CryAudio
