// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Debug.h"

namespace CryAudio
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
DebugColor const g_colorWhite {
	{
		1.0f, 1.0f, 1.0f, 1.0f } };

DebugColor const g_colorRed {
	{
		1.0f, 0.0f, 0.0f, 1.0f } };

DebugColor const g_colorGreen {
	{
		0.0f, 0.8f, 0.0f, 1.0f } };

DebugColor const g_colorBlue {
	{
		0.4f, 0.4f, 1.0f, 1.0f } };

DebugColor const g_colorGreyBright {
	{
		0.9f, 0.9f, 0.9f, 1.0f } };

DebugColor const g_colorGreyDark {
	{
		0.5f, 0.5f, 0.5f, 1.0f } };

DebugColor const g_colorOrange {
	{
		9.0f, 0.5f, 0.0f, 1.0f } };

DebugColor const g_colorYellow {
	{
		0.9f, 0.9f, 0.0f, 1.0f } };

DebugColor const g_colorCyan {
	{
		0.1f, 0.8f, 0.8f, 1.0f } };

// Global debug draw style.
DebugColor const g_globalColorHeader = g_colorOrange;
DebugColor const g_globalColorVirtual = g_colorCyan;
DebugColor const g_globalColorInactive = g_colorGreyDark;

// Debug draw style for system info.
float const g_systemHeaderFontSize = 1.5f;
float const g_systemHeaderLineHeight = 20.0f;
float const g_systemFontSize = 1.35f;
float const g_systemLineHeight = 16.0f;
DebugColor const g_systemColorTextPrimary = g_colorGreyBright;
DebugColor const g_systemColorTextSecondary = g_colorBlue;

// Debug draw style for objects.
float const g_objectFontSize = 1.35f;
float const g_objectLineHeight = 14.0f;
float const g_objectRadiusPositionSphere = 0.15f;
DebugColor const g_objectColorActive = g_colorGreyBright;
DebugColor const g_objectColorTrigger = g_colorGreen;
DebugColor const g_objectColorStandaloneFile = g_colorYellow;
DebugColor const g_objectColorParameter = g_colorBlue;
DebugColor const g_objectColorEnvironment = g_colorOrange;

// Debug draw style for object manager, event manager, standalone file manager and file cache manager.
float const g_managerHeaderFontSize = 1.5f;
float const g_managerHeaderLineHeight = 16.0f;
float const g_managerFontSize = 1.25f;
float const g_managerLineHeight = 11.0f;
DebugColor const g_managerColorItemActive = g_colorGreen;
DebugColor const g_managerColorItemLoading = g_colorRed;
DebugColor const g_managerColorItemStopping = g_colorYellow;

// Debug draw style for rays.
float const g_rayRadiusCollisionSphere = 0.01f;
}      // namespace Debug
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
