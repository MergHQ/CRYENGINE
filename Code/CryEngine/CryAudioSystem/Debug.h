// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryAudio/IAudioInterfacesCommonData.h>
	#include <CryMath/Cry_Color.h>

namespace CryAudio
{
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
	ImplList               = BIT(27), // v
	ActiveObjects          = BIT(28), // w
	FileCacheManagerInfo   = BIT(29), // x
	RequestInfo            = BIT(30), // y
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

// Debug draw style for objects.
static constexpr float s_objectRadiusPositionSphere = 0.15f;
static constexpr float s_objectColorSwitchGreenMin = 0.3f;
static constexpr float s_objectColorSwitchGreenMax = 1.0f;
static constexpr float s_objectColorSwitchUpdateValue = (s_objectColorSwitchGreenMax - s_objectColorSwitchGreenMin) / 100.0f;
static ColorF const s_objectColorPositionSphere = Col_Red;
static ColorF const s_objectColorOcclusionOffsetSphere = { Col_LimeGreen, 0.4f };
static constexpr char const* s_szOcclusionTypes[] = { "None", "Ignore", "Adaptive", "Low", "Medium", "High" };

// Debug draw style for rays.
static constexpr float s_rayRadiusCollisionSphere = 0.01f;
static ColorF const s_rayColorObstructed = Col_Red;
static ColorF const s_rayColorFree = Col_LimeGreen;
static ColorF const s_rayColorCollisionSphere = Col_Orange;

// Debug draw style for file cache manager.
static ColorF const s_afcmColorScopeGlobal = Col_LimeGreen;
static ColorF const s_afcmColorScopeLevelSpecific = Col_Yellow;
static ColorF const s_afcmColorFileLoading = Col_OrangeRed;
static ColorF const s_afcmColorFileMemAllocFail = Col_VioletRed;
static ColorF const s_afcmColorFileRemovable = Col_Cyan;
static ColorF const s_afcmColorFileNotCached = Col_Grey;
static ColorF const s_afcmColorFileNotFound = Col_Red;
}      // namespace Debug
}      // namespace CryAudio
#endif // INCLUDE_AUDIO_PRODUCTION_CODE