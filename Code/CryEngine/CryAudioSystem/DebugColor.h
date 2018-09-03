// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <array>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
using DebugColor = std::array<float, 4>;

extern DebugColor const g_colorWhite;
extern DebugColor const g_colorRed;
extern DebugColor const g_colorGreen;
extern DebugColor const g_colorBlue;
extern DebugColor const g_colorGreyBright;
extern DebugColor const g_colorOrange;
extern DebugColor const g_colorYellow;
extern DebugColor const g_colorCyan;

// Global debug draw style.
extern DebugColor const g_globalColorHeader;
extern DebugColor const g_globalColorVirtual;
extern DebugColor const g_globalColorInactive;

// Debug draw style for audio system info.
extern float const g_systemHeaderFontSize;
extern float const g_systemHeaderLineHeight;
extern float const g_systemFontSize;
extern float const g_systemLineHeight;
extern DebugColor const g_systemColorTextPrimary;
extern DebugColor const g_systemColorTextSecondary;

// Debug draw style for audio objects.
extern float const g_objectFontSize;
extern float const g_objectLineHeight;
extern float const g_objectRadiusPositionSphere;
extern DebugColor const g_objectColorActive;
extern DebugColor const g_objectColorTrigger;
extern DebugColor const g_objectColorStandaloneFile;
extern DebugColor const g_objectColorParameter;
extern DebugColor const g_objectColorEnvironment;

// Debug draw style for audio object manager, audio event manager, standalone file manager and file cache manager.
extern float const g_managerHeaderFontSize;
extern float const g_managerHeaderLineHeight;
extern float const g_managerFontSize;
extern float const g_managerLineHeight;
extern DebugColor const g_managerColorItemActive;
extern DebugColor const g_managerColorItemLoading;
extern DebugColor const g_managerColorItemStopping;

// Debug draw style for rays.
extern float const g_rayRadiusCollisionSphere;
}      // namespace Debug
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
