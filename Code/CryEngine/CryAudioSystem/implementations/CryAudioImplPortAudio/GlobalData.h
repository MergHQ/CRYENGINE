// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
constexpr char const* g_szImplFolderName = "portaudio";

// XML tags
constexpr char const* g_szEventTag = "Event";
constexpr char const* g_szSampleTag = "Sample";

// XML attributes
constexpr char const* g_szPathAttribute = "path";
constexpr char const* g_szLoopCountAttribute = "loop_count";
constexpr char const* g_szLocalizedAttribute = "localized";

// XML attributes for impl data node
constexpr char const* g_szEventsAttribute = "events";

// XML values
constexpr char const* g_szTrueValue = "true";
constexpr char const* g_szStartValue = "start";
constexpr char const* g_szStopValue = "stop";

static std::unordered_map<char const*, char const*> const g_supportedExtensions
{
	{ "wav", "Wave (Microsoft)" } };

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	char name[MaxFileNameLength] = { '\0' };
	char path[MaxFilePathLength] = { '\0' };
	bool isLocalized = false;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
