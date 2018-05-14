// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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

// Debug draw style for audio system info.
extern float const g_systemFontSize;
extern float const g_systemLineHeight;
extern float const g_systemLineHeightClause;
extern float const g_systemIndentation;
extern DebugColor const g_systemColorHeader;
extern DebugColor const g_systemColorTextPrimary;
extern DebugColor const g_systemColorTextSecondary;
extern DebugColor const g_systemColorListenerActive;
extern DebugColor const g_systemColorListenerInactive;

// Debug draw style for audio objects.
extern float const g_objectFontSize;
extern float const g_objectLineHeight;
extern float const g_objectRadiusPositionSphere;
extern DebugColor const g_objectColorActive;
extern DebugColor const g_objectColorInactive;
extern DebugColor const g_objectColorVirtual;
extern DebugColor const g_objectColorTrigger;
extern DebugColor const g_objectColorStandaloneFile;
extern DebugColor const g_objectColorParameter;
extern DebugColor const g_objectColorEnvironment;

// Debug draw style for audio object manager, audio event manager, standalone file manager and file cache manager.
extern float const g_managerHeaderFontSize;
extern float const g_managerHeaderLineHeight;
extern float const g_managerFontSize;
extern float const g_managerLineHeight;
extern DebugColor const g_managerColorHeader;
extern DebugColor const g_managerColorItemActive;
extern DebugColor const g_managerColorItemInactive;
extern DebugColor const g_managerColorItemVirtual;
extern DebugColor const g_managerColorItemLoading;
extern DebugColor const g_managerColorItemStopping;

// Debug draw style for rays.
extern float const g_rayRadiusCollisionSphere;
}      // namespace Debug
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
