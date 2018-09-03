// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include <array>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
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

// Debug draw style for system info.
extern float const g_debugSystemHeaderFontSize;
extern float const g_debugSystemFontSize;
extern float const g_debugSystemLineHeight;
extern float const g_debugSystemLineHeightClause;
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
}      // namespace Adx2
}      // namespace Impl
}      // namespace CryAudio
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
