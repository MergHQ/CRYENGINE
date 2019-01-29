// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Cry_Color.h>

namespace CryAudio
{
namespace Debug
{
// Global debug draw style.
static ColorF const s_globalColorError = Col_Red;
static ColorF const s_globalColorHeader = Col_Orange;
static ColorF const s_globalColorVirtual = Col_Cyan;
static ColorF const s_globalColorInactive = Col_Grey;

// Debug draw style for system information.
constexpr float g_systemHeaderFontSize = 1.5f;
constexpr float g_systemHeaderLineHeight = 20.0f;
constexpr float g_systemHeaderLineSpacerHeight = 4.0f;
constexpr float g_systemFontSize = 1.35f;
constexpr float g_systemLineHeight = 16.0f;
static ColorF const s_systemColorTextPrimary = Col_LightGrey;
static ColorF const s_systemColorTextSecondary = Col_CornflowerBlue;
static ColorF const s_systemColorListenerActive = Col_LimeGreen;
static ColorF const s_systemColorListenerInactive = Col_OrangeRed;

// Debug draw style for objects.
constexpr float g_objectFontSize = 1.35f;
constexpr float g_objectLineHeight = 14.0f;
static ColorF const s_objectColorActive = Col_LightGrey;
static ColorF const s_objectColorTrigger = Col_LimeGreen;
static ColorF const s_objectColorStandaloneFile = Col_Yellow;
static ColorF const s_objectColorParameter = Col_SlateBlue;
static ColorF const s_objectColorEnvironment = Col_Orange;

// Debug draw style for lists.
constexpr float g_listHeaderFontSize = 1.5f;
constexpr float g_listHeaderLineHeight = 16.0f;
constexpr float g_listFontSize = 1.25f;
constexpr float g_listLineHeight = 11.0f;
static ColorF const s_listColorItemActive = Col_LimeGreen;
static ColorF const s_listColorItemLoading = Col_OrangeRed;
static ColorF const s_listColorItemStopping = Col_Yellow;
} // namespace Debug
} // namespace CryAudio
