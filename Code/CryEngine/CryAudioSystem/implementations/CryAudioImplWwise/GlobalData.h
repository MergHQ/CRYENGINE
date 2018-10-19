// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
static constexpr char const* s_szImplFolderName = "wwise";

// XML tags
static constexpr char const* s_szParameterTag = "Parameter";
static constexpr char const* s_szSwitchGroupTag = "SwitchGroup";
static constexpr char const* s_szStateGroupTag = "StateGroup";
static constexpr char const* s_szFileTag = "File";
static constexpr char const* s_szAuxBusTag = "AuxBus";
static constexpr char const* s_szValueTag = "Value";

// XML attributes
static constexpr char const* s_szValueAttribute = "value";
static constexpr char const* s_szMutiplierAttribute = "value_multiplier";
static constexpr char const* s_szShiftAttribute = "value_shift";
static constexpr char const* s_szLocalizedAttribute = "localized";

// XML attributes for impl data node
static constexpr char const* s_szTriggersAttribute = "triggers";
static constexpr char const* s_szParametersAttribute = "parameters";
static constexpr char const* s_szSwitchStatesAttribute = "switchstates";
static constexpr char const* s_szEnvironmentsAttribute = "environments";
static constexpr char const* s_szFilesAttribute = "files";

// XML values
static constexpr char const* s_szTrueValue = "true";

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
	uint32 triggers = 0;
	uint32 parameters = 0;
	uint32 switchStates = 0;
	uint32 environments = 0;
	uint32 files = 0;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
