// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
constexpr char const* g_szImplFolderName = "wwise";

// XML tags
constexpr char const* g_szEventTag = "Event";
constexpr char const* g_szParameterTag = "Parameter";
constexpr char const* g_szSwitchGroupTag = "SwitchGroup";
constexpr char const* g_szStateGroupTag = "StateGroup";
constexpr char const* g_szFileTag = "File";
constexpr char const* g_szAuxBusTag = "AuxBus";
constexpr char const* g_szValueTag = "Value";

// XML attributes
constexpr char const* g_szValueAttribute = "value";
constexpr char const* g_szMutiplierAttribute = "value_multiplier";
constexpr char const* g_szShiftAttribute = "value_shift";
constexpr char const* g_szLocalizedAttribute = "localized";

// XML attributes for impl data node
constexpr char const* g_szEventsAttribute = "events";
constexpr char const* g_szParametersAttribute = "parameters";
constexpr char const* g_szParametersAdvancedAttribute = "parametersadvanced";
constexpr char const* g_szParameterEnvironmentsAttribute = "parameterenvironments";
constexpr char const* g_szParameterEnvironmentsAdvancedAttribute = "parameterenvironmentsadvanced";
constexpr char const* g_szParameterStatesAttribute = "parameterstates";
constexpr char const* g_szStatesAttribute = "states";
constexpr char const* g_szSwitchesAttribute = "switches";
constexpr char const* g_szAuxBusesAttribute = "auxbuses";
constexpr char const* g_szSoundBanksAttribute = "soundbanks";

// XML values
constexpr char const* g_szTrueValue = "true";

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
	uint16 states = 0;
	uint16 switches = 0;
	uint16 auxBuses = 0;
	uint16 soundBanks = 0;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
