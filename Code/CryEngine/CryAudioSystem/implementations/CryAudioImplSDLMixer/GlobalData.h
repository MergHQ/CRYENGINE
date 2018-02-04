// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
static constexpr char* s_szImplFolderName = "sdlmixer";

// XML tags
static constexpr char* s_szFileTag = "Sample";

// XML attributes
static constexpr char* s_szPathAttribute = "path";
static constexpr char* s_szPanningEnabledAttribute = "enable_panning";
static constexpr char* s_szAttenuationEnabledAttribute = "enable_dist_attenuation";
static constexpr char* s_szAttenuationMinDistanceAttribute = "attenuation_dist_min";
static constexpr char* s_szAttenuationMaxDistanceAttribute = "attenuation_dist_max";
static constexpr char* s_szVolumeAttribute = "volume";
static constexpr char* s_szLoopCountAttribute = "loop_count";

// XML values
static constexpr char* s_szTrueValue = "true";
static constexpr char* s_szFalseValue = "false";
static constexpr char* s_szStartValue = "start";
static constexpr char* s_szStopValue = "stop";
static constexpr char* s_szPauseValue = "pause";
static constexpr char* s_szResumeValue = "resume";
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio
