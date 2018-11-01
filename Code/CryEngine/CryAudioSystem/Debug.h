// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryAudio/IAudioInterfacesCommonData.h>
	#include <array>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
// Filter for drawing debug info to the screen
enum class EDrawFilter : EnumFlagsType
{
	All                    = 0,
	Spheres                = BIT(6),  // a
	ObjectLabel            = BIT(7),  // b
	ObjectTriggers         = BIT(8),  // c
	ObjectStates           = BIT(9),  // d
	ObjectParameters       = BIT(10), // e
	ObjectEnvironments     = BIT(11), // f
	ObjectDistance         = BIT(12), // g
	OcclusionRayLabels     = BIT(13), // h
	OcclusionRays          = BIT(14), // i
	OcclusionRayOffset     = BIT(15), // j
	ListenerOcclusionPlane = BIT(16), // k
	ObjectStandaloneFiles  = BIT(17), // l
	ObjectImplInfo         = BIT(18), // m

	HideMemoryInfo         = BIT(22), // q
	FilterAllObjectInfo    = BIT(23), // r
	DetailedMemoryInfo     = BIT(24), // s

	StandaloneFiles        = BIT(26), // u
	ActiveEvents           = BIT(27), // v
	ActiveObjects          = BIT(28), // w
	FileCacheManagerInfo   = BIT(29), // x
};
CRY_CREATE_ENUM_FLAG_OPERATORS(EDrawFilter);

static constexpr EDrawFilter objectMask =
	EDrawFilter::Spheres |
	EDrawFilter::ObjectLabel |
	EDrawFilter::ObjectTriggers |
	EDrawFilter::ObjectStates |
	EDrawFilter::ObjectParameters |
	EDrawFilter::ObjectEnvironments |
	EDrawFilter::ObjectDistance |
	EDrawFilter::OcclusionRayLabels |
	EDrawFilter::OcclusionRays |
	EDrawFilter::OcclusionRayOffset |
	EDrawFilter::ListenerOcclusionPlane |
	EDrawFilter::ObjectStandaloneFiles |
	EDrawFilter::ObjectImplInfo;

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

// Debug draw style for system info.
extern float const g_systemHeaderFontSize;
extern float const g_systemHeaderLineHeight;
extern float const g_systemFontSize;
extern float const g_systemLineHeight;
extern DebugColor const g_systemColorTextPrimary;
extern DebugColor const g_systemColorTextSecondary;

// Debug draw style for objects.
extern float const g_objectFontSize;
extern float const g_objectLineHeight;
extern float const g_objectRadiusPositionSphere;
extern DebugColor const g_objectColorActive;
extern DebugColor const g_objectColorTrigger;
extern DebugColor const g_objectColorStandaloneFile;
extern DebugColor const g_objectColorParameter;
extern DebugColor const g_objectColorEnvironment;

// Debug draw style for object manager, event manager, standalone file manager and file cache manager.
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
