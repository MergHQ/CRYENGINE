// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <map>

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
struct IEnvironmentConnection;
struct IParameterConnection;
struct ISettingConnection;
struct ISwitchStateConnection;
struct ITriggerConnection;
} // namespace Impl

class CSystem;
class CObject;
class CLoseFocusTrigger;
class CGetFocusTrigger;
class CMuteAllTrigger;
class CUnmuteAllTrigger;
class CPauseAllTrigger;
class CResumeAllTrigger;
class CTrigger;
class CParameter;
class CSwitch;
class CPreloadRequest;
class CEnvironment;
class CSetting;

enum class ESystemStates : EnumFlagsType
{
	None             = 0,
	ImplShuttingDown = BIT(0),
	IsMuted          = BIT(1),
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	IsPaused         = BIT(2),
	PoolsAllocated   = BIT(3),
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
};
CRY_CREATE_ENUM_FLAG_OPERATORS(ESystemStates);

using TriggerLookup = std::map<ControlId, CTrigger const*>;
using ParameterLookup = std::map<ControlId, CParameter const*>;
using SwitchLookup = std::map<ControlId, CSwitch const*>;
using PreloadRequestLookup = std::map<PreloadRequestId, CPreloadRequest*>;
using EnvironmentLookup = std::map<EnvironmentId, CEnvironment const*>;
using SettingLookup = std::map<ControlId, CSetting const*>;
using TriggerInstanceIdLookup = std::map<TriggerInstanceId, CObject*>;

using TriggerConnections = std::vector<Impl::ITriggerConnection*>;
using ParameterConnections = std::vector<Impl::IParameterConnection*>;
using SwitchStateConnections = std::vector<Impl::ISwitchStateConnection*>;
using EnvironmentConnections = std::vector<Impl::IEnvironmentConnection*>;
using SettingConnections = std::vector<Impl::ISettingConnection*>;

extern Impl::IImpl* g_pIImpl;
extern CSystem g_system;
extern ESystemStates g_systemStates;
extern TriggerLookup g_triggers;
extern ParameterLookup g_parameters;
extern SwitchLookup g_switches;
extern PreloadRequestLookup g_preloadRequests;
extern EnvironmentLookup g_environments;
extern SettingLookup g_settings;
extern TriggerInstanceIdLookup g_triggerInstanceIdToObject;
extern CObject* g_pObject;
extern CLoseFocusTrigger g_loseFocusTrigger;
extern CGetFocusTrigger g_getFocusTrigger;
extern CMuteAllTrigger g_muteAllTrigger;
extern CUnmuteAllTrigger g_unmuteAllTrigger;
extern CPauseAllTrigger g_pauseAllTrigger;
extern CResumeAllTrigger g_resumeAllTrigger;

extern SImplInfo g_implInfo;
extern CryFixedStringT<MaxFilePathLength> g_configPath;

extern TriggerInstanceId g_triggerInstanceIdCounter;
constexpr TriggerInstanceId g_maxTriggerInstanceId = std::numeric_limits<TriggerInstanceId>::max();

struct SPoolSizes final
{
	uint16 triggers = 0;
	uint16 parameters = 0;
	uint16 switches = 0;
	uint16 states = 0;
	uint16 environments = 0;
	uint16 preloads = 0;
	uint16 settings = 0;
	uint16 files = 0;
};

extern SPoolSizes g_poolSizes;

static void IncrementTriggerInstanceIdCounter()
{
	if (g_triggerInstanceIdCounter == g_maxTriggerInstanceId)
	{
		// Set to 1 because 0 is an invalid id.
		g_triggerInstanceIdCounter = 1;
	}
	else
	{
		++g_triggerInstanceIdCounter;
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
static constexpr char const* s_szPreviewTriggerName = "preview_trigger";
static constexpr ControlId PreviewTriggerId = StringToId(s_szPreviewTriggerName);

class CPreviewTrigger;
extern CPreviewTrigger g_previewTrigger;
extern CObject g_previewObject;
extern SPoolSizes g_debugPoolSizes;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
