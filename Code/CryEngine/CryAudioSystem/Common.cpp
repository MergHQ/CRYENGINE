// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Common.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <array>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
namespace Debug
{
DebugColor const g_colorWhite {
	{
		1.0f, 1.0f, 1.0f, 0.9f
	}
};

DebugColor const g_colorRed {
	{
		1.0f, 0.0f, 0.0f, 0.7f
	}
};

DebugColor const g_colorGreen {
	{
		0.0f, 0.8f, 0.0f, 0.7f
	}
};

DebugColor const g_colorBlue {
	{
		0.4f, 0.4f, 1.0f, 1.0f
	}
};

DebugColor const g_colorGreyBright {
	{
		0.9f, 0.9f, 0.9f, 0.9f
	}
};

DebugColor const g_colorGreyDark {
	{
		0.5f, 0.5f, 0.5f, 0.9f
	}
};

DebugColor const g_colorOrange {
	{
		9.0f, 0.5f, 0.0f, 0.7f
	}
};

DebugColor const g_colorYellow {
	{
		0.9f, 0.9f, 0.0f, 0.7f
	}
};

DebugColor const g_colorCyan {
	{
		0.1f, 0.8f, 0.8f, 0.9f
	}
};

// Debug draw style for audio system info.
float const g_systemFontSize = 1.35f;
float const g_systemLineHeight = 16.0f;
float const g_systemLineHeightClause = 20.0f;
float const g_systemIndentation = 24.0f;
DebugColor const g_systemColorHeader = g_colorBlue;
DebugColor const g_systemColorTextPrimary = g_colorWhite;
DebugColor const g_systemColorTextSecondary = g_colorGreyBright;
DebugColor const g_systemColorListenerActive = g_colorGreen;
DebugColor const g_systemColorListenerInactive = g_colorRed;

// Debug draw style for audio objects.
float const g_objectFontSize = 1.35f;
float const g_objectLineHeight = 14.0f;
float const g_objectRadiusPositionSphere = 0.15f;
DebugColor const g_objectColorActive = g_colorGreyBright;
DebugColor const g_objectColorInactive = g_colorGreyDark;
DebugColor const g_objectColorVirtual = g_colorCyan;
DebugColor const g_objectColorTrigger = g_colorGreen;
DebugColor const g_objectColorStandaloneFile = g_colorYellow;
DebugColor const g_objectColorParameter = g_colorBlue;
DebugColor const g_objectColorEnvironment = g_colorOrange;

// Debug draw style for audio object manager, audio event manager, standalone file manager and file cache manager.
float const g_managerHeaderFontSize = 1.5f;
float const g_managerHeaderLineHeight = 16.0f;
float const g_managerFontSize = 1.25f;
float const g_managerLineHeight = 11.0f;
DebugColor const g_managerColorHeader = g_colorOrange;
DebugColor const g_managerColorItemActive = g_colorGreen;
DebugColor const g_managerColorItemInactive = g_colorGreyDark;
DebugColor const g_managerColorItemVirtual = g_colorCyan;
DebugColor const g_managerColorItemLoading = g_colorRed;
DebugColor const g_managerColorItemStopping = g_colorYellow;

// Debug draw style for rays.
float const g_rayRadiusCollisionSphere = 0.01f;
}      // namespace Debug
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
