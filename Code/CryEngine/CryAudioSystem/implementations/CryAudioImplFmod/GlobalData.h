// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
static constexpr char const* s_szImplFolderName = "fmod";

// XML tags
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
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
