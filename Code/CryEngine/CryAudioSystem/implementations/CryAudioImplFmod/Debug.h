// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_FMOD_IMPL_PRODUCTION_CODE)
	#include <array>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
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

// Debug draw style for system info.
extern float const g_debugSystemHeaderFontSize;
extern float const g_debugSystemHeaderLineHeight;
extern float const g_debugSystemFontSize;
extern float const g_debugSystemLineHeight;
extern DebugColor const g_debugSystemColorHeader;
extern DebugColor const g_debugSystemColorTextPrimary;
extern DebugColor const g_debugSystemColorTextSecondary;
extern DebugColor const g_debugSystemColorListenerActive;
extern DebugColor const g_debugSystemColorListenerInactive;

// Debug draw style for objects.
extern float const g_debugObjectFontSize;
extern float const g_debugObjectLineHeight;
extern DebugColor const g_debugObjectColorVirtual;
extern DebugColor const g_debugObjectColorPhysical;
}      // namespace Fmod
}      // namespace Impl
}      // namespace CryAudio
#endif // INCLUDE_FMOD_IMPL_PRODUCTION_CODE
