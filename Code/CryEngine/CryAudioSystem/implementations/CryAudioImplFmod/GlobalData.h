// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
static constexpr char const* s_szImplFolderName = "fmod";

// XML tags
static constexpr char const* s_szEventTag = "Event";
static constexpr char const* s_szKeyTag = "Key";
static constexpr char const* s_szSnapshotTag = "Snapshot";
static constexpr char const* s_szParameterTag = "Parameter";
static constexpr char const* s_szFileTag = "File";
static constexpr char const* s_szBusTag = "Bus";
static constexpr char const* s_szVcaTag = "VCA";

// XML attributes
static constexpr char const* s_szValueAttribute = "value";
static constexpr char const* s_szMutiplierAttribute = "value_multiplier";
static constexpr char const* s_szShiftAttribute = "value_shift";
static constexpr char const* s_szLocalizedAttribute = "localized";
static constexpr char const* s_szEventAttribute = "event";

// XML attributes for impl data node
static constexpr char const* s_szTriggersAttribute = "triggers";
static constexpr char const* s_szParametersAttribute = "parameters";
static constexpr char const* s_szSwitchStatesAttribute = "switchstates";
static constexpr char const* s_szEnvBusesAttribute = "env_buses";
static constexpr char const* s_szEnvParametersAttribute = "env_parameters";
static constexpr char const* s_szVcaParametersAttribute = "vca_parameters";
static constexpr char const* s_szVcaStatesAttribute = "vca_states";
static constexpr char const* s_szFilesAttribute = "files";

// XML values
static constexpr char const* s_szTrueValue = "true";
static constexpr char const* s_szStopValue = "stop";
static constexpr char const* s_szPauseValue = "pause";
static constexpr char const* s_szResumeValue = "resume";

// Default values
static constexpr float const s_defaultParamMultiplier = 1.0f;
static constexpr float const s_defaultParamShift = 0.0f;
static constexpr float const s_defaultStateValue = 0.0f;

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	CryFixedStringT<MaxControlNameLength> name;
};

struct SPoolSizes final
{
	uint16 triggers = 0;
	uint16 parameters = 0;
	uint16 switchStates = 0;
	uint16 envBuses = 0;
	uint16 envParameters = 0;
	uint16 vcaParameters = 0;
	uint16 vcaStates = 0;
	uint16 files = 0;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
