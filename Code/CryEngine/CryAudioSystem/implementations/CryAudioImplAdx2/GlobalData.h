// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
constexpr char const* g_szImplFolderName = "adx2";

// XML tags
constexpr char const* g_szCueTag = "Cue";
constexpr char const* g_szBinaryTag = "Binary";
constexpr char const* g_szAisacControlTag = "AisacControl";
constexpr char const* g_szGameVariableTag = "GameVariable";
constexpr char const* g_szSelectorTag = "Selector";
constexpr char const* g_szSelectorLabelTag = "SelectorLabel";
constexpr char const* g_szSCategoryTag = "Category";
constexpr char const* g_szDspBusSettingTag = "DspBusSetting";
constexpr char const* g_szDspBusTag = "DspBus";
constexpr char const* g_szSnapshotTag = "Snapshot";

// XML attributes
constexpr char const* g_szCueSheetAttribute = "cuesheet";
constexpr char const* g_szValueAttribute = "value";
constexpr char const* g_szValueMinAttribute = "value_min";
constexpr char const* g_szValueMaxAttribute = "value_max";
constexpr char const* g_szMutiplierAttribute = "value_multiplier";
constexpr char const* g_szShiftAttribute = "value_shift";
constexpr char const* g_szLocalizedAttribute = "localized";
constexpr char const* g_szTimeAttribute = "time";

// XML attributes for impl data node
constexpr char const* g_szCuesAttribute = "cues";
constexpr char const* g_szAisacControlsAttribute = "aisaccontrols";
constexpr char const* g_szAisacControlsAdvancedAttribute = "aisaccontrolsadvanced";
constexpr char const* g_szAisacEnvironmentsAttribute = "aisacenvironments";
constexpr char const* g_szAisacEnvironmentsAdvancedAttribute = "aisacenvironmentsadvanced";
constexpr char const* g_szAisacStatesAttribute = "aisacstates";
constexpr char const* g_szCategoriesAttribute = "categories";
constexpr char const* g_szCategoriesAdvancedAttribute = "categoriesadvanced";
constexpr char const* g_szCategoryStatesAttribute = "categorystates";
constexpr char const* g_szGameVariablesAttribute = "gamevariables";
constexpr char const* g_szGameVariablesAdvancedAttribute = "gamevariablesadvanced";
constexpr char const* g_szGameVariableStatesAttribute = "gamevariablestates";
constexpr char const* g_szSelectorLabelsAttribute = "selectorlabels";
constexpr char const* g_szDspBusesAttribute = "dspbuses";
constexpr char const* g_szSnapshotsAttribute = "snapshots";
constexpr char const* g_szDspBusSettingsAttribute = "dspbussettings";
constexpr char const* g_szBinariesAttribute = "binaries";

// XML values
constexpr char const* g_szTrueValue = "true";
constexpr char const* g_szStopValue = "stop";
constexpr char const* g_szPauseValue = "pause";
constexpr char const* g_szResumeValue = "resume";

// Default values
constexpr float g_defaultParamMinValue = 0.0f;
constexpr float g_defaultParamMaxValue = 1.0f;
constexpr float g_defaultParamMultiplier = 1.0f;
constexpr float g_defaultParamShift = 0.0f;
constexpr float g_defaultStateValue = 0.0f;
constexpr float g_defaultCategoryVolume = 1.0f;
constexpr int g_defaultChangeoverTime = 0;

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	char name[MaxControlNameLength] = { '\0' };
	char cueSheet[MaxControlNameLength] = { '\0' };
};

struct SPoolSizes final
{
	uint16 cues = 0;
	uint16 aisacControls = 0;
	uint16 aisacControlsAdvanced = 0;
	uint16 aisacEnvironments = 0;
	uint16 aisacEnvironmentsAdvanced = 0;
	uint16 aisacStates = 0;
	uint16 categories = 0;
	uint16 categoriesAdvanced = 0;
	uint16 categoryStates = 0;
	uint16 gameVariables = 0;
	uint16 gameVariablesAdvanced = 0;
	uint16 gameVariableStates = 0;
	uint16 selectorLabels = 0;
	uint16 dspBuses = 0;
	uint16 snapshots = 0;
	uint16 dspBusSettings = 0;
	uint16 binaries = 0;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
