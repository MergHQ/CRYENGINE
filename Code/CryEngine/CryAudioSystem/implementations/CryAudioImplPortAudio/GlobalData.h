// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IAudioImpl.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
static constexpr char const* s_szImplFolderName = "portaudio";

// XML tags
static constexpr char const* s_szFileTag = "Sample";

// XML attributes
static constexpr char const* s_szPathAttribute = "path";
static constexpr char const* s_szLoopCountAttribute = "loop_count";
static constexpr char const* s_szLocalizedAttribute = "localized";

// XML values
static constexpr char const* s_szTrueValue = "true";
static constexpr char const* s_szStartValue = "start";
static constexpr char const* s_szStopValue = "stop";

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	CryFixedStringT<MaxFileNameLength> name;
	CryFixedStringT<MaxFilePathLength> path;
	bool                               isLocalized;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
