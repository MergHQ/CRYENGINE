// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE)
	#include <array>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
struct SMemoryInfo final
{
	size_t totalMemory;            // total amount of memory used by the implementation in bytes
	size_t poolUsedObjects;        // total number of active pool objects
	size_t poolConstructedObjects; // total number of constructed pool objects
	size_t poolUsedMemory;         // memory used by the constructed objects
	size_t poolAllocatedMemory;    // total memory allocated by the pool allocator
};

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
extern float const g_debugSystemHeaderFontSize;
extern float const g_debugSystemFontSize;
extern float const g_debugSystemLineHeight;
extern float const g_debugSystemLineHeightClause;
extern DebugColor const g_debugSystemColorHeader;
extern DebugColor const g_debugSystemColorTextPrimary;
extern DebugColor const g_debugSystemColorTextSecondary;
extern DebugColor const g_debugSystemColorListenerActive;
extern DebugColor const g_debugSystemColorListenerInactive;
}      // namespace SDL_mixer
}      // namespace Impl
}      // namespace CryAudio
#endif // INCLUDE_SDLMIXER_IMPL_PRODUCTION_CODE
