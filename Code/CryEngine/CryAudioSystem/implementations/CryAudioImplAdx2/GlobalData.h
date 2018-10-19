// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
static constexpr char const* s_szImplFolderName = "adx2";

// XML tags
static constexpr char const* s_szCueTag = "Cue";
static constexpr char const* s_szFileTag = "File";
static constexpr char const* s_szAisacControlTag = "AisacControl";
static constexpr char const* s_szGameVariableTag = "GameVariable";
static constexpr char const* s_szSelectorTag = "Selector";
static constexpr char const* s_szSelectorLabelTag = "SelectorLabel";
static constexpr char const* s_szSCategoryTag = "Category";
static constexpr char const* s_szDspBusSettingTag = "DspBusSetting";
static constexpr char const* s_szBusTag = "Bus";
static constexpr char const* s_szSnapshotTag = "Snapshot";

// XML attributes
static constexpr char const* s_szCueSheetAttribute = "cuesheet";
static constexpr char const* s_szValueAttribute = "value";
static constexpr char const* s_szMutiplierAttribute = "value_multiplier";
static constexpr char const* s_szShiftAttribute = "value_shift";
static constexpr char const* s_szLocalizedAttribute = "localized";
static constexpr char const* s_szTimeAttribute = "time";

// XML attributes for impl data node
static constexpr char const* s_szTriggersAttribute = "triggers";
static constexpr char const* s_szParametersAttribute = "parameters";
static constexpr char const* s_szSwitchStatesAttribute = "switchstates";
static constexpr char const* s_szEnvironmentsAttribute = "environments";
static constexpr char const* s_szSettingsAttribute = "settings";
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
static constexpr float const s_defaultCategoryVolume = 1.0f;
static constexpr int const s_defaultChangeoverTime = 0;

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	CryFixedStringT<MaxControlNameLength> name;
	CryFixedStringT<MaxControlNameLength> cueSheet;
};

struct SPoolSizes final
{
	uint32 triggers = 0;
	uint32 parameters = 0;
	uint32 switchStates = 0;
	uint32 environments = 0;
	uint32 settings = 0;
	uint32 files = 0;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
