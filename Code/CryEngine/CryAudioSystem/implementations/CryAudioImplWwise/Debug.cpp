// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#if defined(INCLUDE_WWISE_IMPL_PRODUCTION_CODE)
	#include "Debug.h"

namespace CryAudio
{
namespace Impl
{
namespace Wwise
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

// Debug draw style for system info.
float const g_debugSystemHeaderFontSize = 1.5f;
float const g_debugSystemHeaderLineHeight = 20.0f;
float const g_debugSystemFontSize = 1.35f;
float const g_debugSystemLineHeight = 16.0f;
DebugColor const g_debugSystemColorHeader = g_colorOrange;
DebugColor const g_debugSystemColorTextPrimary = g_colorGreyBright;
DebugColor const g_debugSystemColorTextSecondary = g_colorBlue;
DebugColor const g_debugSystemColorListenerActive = g_colorGreen;
DebugColor const g_debugSystemColorListenerInactive = g_colorRed;

// Debug draw style for objects.
float const g_debugObjectFontSize = 1.35f;
float const g_debugObjectLineHeight = 14.0f;
DebugColor const g_debugObjectColorVirtual = g_colorCyan;
DebugColor const g_debugObjectColorPhysical = g_colorYellow;
}      // namespace Wwise
}      // namespace Impl
}      // namespace CryAudio
#endif // INCLUDE_WWISE_IMPL_PRODUCTION_CODE
