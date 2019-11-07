// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>
#include <ITriggerInfo.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
constexpr char const* g_szImplFolderName = "sdlmixer";

// XML tags
constexpr char const* g_szEventTag = "Event";
constexpr char const* g_szSampleTag = "Sample";

// XML attributes
constexpr char const* g_szPathAttribute = "path";
constexpr char const* g_szPanningEnabledAttribute = "enable_panning";
constexpr char const* g_szAttenuationEnabledAttribute = "enable_dist_attenuation";
constexpr char const* g_szAttenuationMinDistanceAttribute = "attenuation_dist_min";
constexpr char const* g_szAttenuationMaxDistanceAttribute = "attenuation_dist_max";
constexpr char const* g_szVolumeAttribute = "volume";
constexpr char const* g_szLoopCountAttribute = "loop_count";
constexpr char const* g_szFadeInTimeAttribute = "fade_in_time";
constexpr char const* g_szFadeOutTimeAttribute = "fade_out_time";
constexpr char const* g_szValueAttribute = "value";
constexpr char const* g_szMutiplierAttribute = "value_multiplier";
constexpr char const* g_szShiftAttribute = "value_shift";
constexpr char const* g_szLocalizedAttribute = "localized";

// XML attributes for impl data node
constexpr char const* g_szEventsAttribute = "events";
constexpr char const* g_szParametersAttribute = "parameters";
constexpr char const* g_szParametersAdvancedAttribute = "parametersadvanced";
constexpr char const* g_szSwitchStatesAttribute = "switchstates";
constexpr char const* g_szFilesAttribute = "files";

// XML values
constexpr char const* g_szTrueValue = "true";
constexpr char const* g_szFalseValue = "false";
constexpr char const* g_szStartValue = "start";
constexpr char const* g_szStopValue = "stop";
constexpr char const* g_szPauseValue = "pause";
constexpr char const* g_szResumeValue = "resume";

// Default values
constexpr float g_defaultFadeInTime = 0.0f;
constexpr float g_defaultFadeOutTime = 0.0f;
constexpr float g_defaultMinAttenuationDist = 0.0f;
constexpr float g_defaultMaxAttenuationDist = 100.0f;
constexpr float g_defaultParamMultiplier = 1.0f;
constexpr float g_defaultParamShift = 0.0f;
constexpr float g_defaultStateValue = 1.0f;

static std::unordered_map<char const*, char const*> const g_supportedExtensions
{
	{ "ogg", "Ogg Vorbis" },
	{ "wav", "Wave (Microsoft)" },
	{ "mp3", "Mpeg Layer 3" },
	{ "opus", "OPUS" },
	{ "flac", "FLAC" },
	{ "mod", "MOD (15 and 31 instruments)" },
	{ "aiff", "APPLE" },
	{ "xm", "FastTracker 2" },
	{ "mid", "MIDI" },
	{ "voc", "VOC" },
	{ "669", "Composer, Unis" },
	{ "amf", "DSMI Advanced Module, ASYLUM Music Format (v1.0)" },
	{ "apun", "APlayer" },
	{ "dsm", "DSIK internal format" },
	{ "far", "Farandole Composer" },
	{ "gdm", "General DigiMusic" },
	{ "it", "Impulse Tracker" },
	{ "imf", "Imago Orpheus" },
	{ "med", "OctaMED" },
	{ "mtm", "MultiTracker Module Editor" },
	{ "okt", "Amiga Oktalyzer" },
	{ "s3m", "Scream Tracker 3" },
	{ "stm", "Scream Tracker" },
	{ "stx", "Scream Tracker Music Interface Kit" },
	{ "ult", "UltraTracker" },
	{ "uni", "MikMod" } };

// Required to create a preview trigger in editor.
struct STriggerInfo final : public ITriggerInfo
{
	char name[MaxFileNameLength] = { '\0' };
	char path[MaxFilePathLength] = { '\0' };
	bool isLocalized = false;
};

struct SPoolSizes final
{
	uint16 events = 0;
	uint16 parameters = 0;
	uint16 parametersAdvanced = 0;
	uint16 switchStates = 0;
	uint16 files = 0;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
