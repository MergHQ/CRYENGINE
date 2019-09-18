// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
constexpr char const* g_szImplFolderName = "fmod";

// XML tags
constexpr char const* g_szEventTag = "Event";
constexpr char const* g_szKeyTag = "Key";
constexpr char const* g_szSnapshotTag = "Snapshot";
constexpr char const* g_szParameterTag = "Parameter";
constexpr char const* g_szFileTag = "File";
constexpr char const* g_szBusTag = "Bus";
constexpr char const* g_szVcaTag = "VCA";

// XML attributes
constexpr char const* g_szValueAttribute = "value";
constexpr char const* g_szMutiplierAttribute = "value_multiplier";
constexpr char const* g_szShiftAttribute = "value_shift";
constexpr char const* g_szLocalizedAttribute = "localized";
constexpr char const* g_szEventAttribute = "event";

// XML attributes for impl data node
constexpr char const* g_szEventsAttribute = "events";
constexpr char const* g_szParametersAttribute = "parameters";
constexpr char const* g_szParametersAdvancedAttribute = "parametersadvanced";
constexpr char const* g_szParameterEnvironmentsAttribute = "parameterenvironments";
constexpr char const* g_szParameterEnvironmentsAdvancedAttribute = "parameterenvironmentsadvanced";
constexpr char const* g_szParameterStatesAttribute = "parameterstates";
constexpr char const* g_szSnapshotsAttribute = "snapshots";
constexpr char const* g_szReturnsAttribute = "returns";
constexpr char const* g_szVcasAttribute = "vcas";
constexpr char const* g_szVcaStatesAttribute = "vcastates";
constexpr char const* g_szBanksAttribute = "banks";

// XML values
constexpr char const* g_szTrueValue = "true";
constexpr char const* g_szStopValue = "stop";
constexpr char const* g_szPauseValue = "pause";
constexpr char const* g_szResumeValue = "resume";

// Default values
constexpr float g_defaultParamMultiplier = 1.0f;
constexpr float g_defaultParamShift = 0.0f;
constexpr float g_defaultStateValue = 0.0f;

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	char name[MaxControlNameLength] = { '\0' };
};

struct SPoolSizes final
{
	uint16 events = 0;
	uint16 parameters = 0;
	uint16 parametersAdvanced = 0;
	uint16 parameterEnvironments = 0;
	uint16 parameterEnvironmentsAdvanced = 0;
	uint16 parameterStates = 0;
	uint16 snapshots = 0;
	uint16 returns = 0;
	uint16 vcas = 0;
	uint16 vcaStates = 0;
	uint16 banks = 0;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
