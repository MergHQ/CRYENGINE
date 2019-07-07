// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "EditorFramework/Preferences.h"
#include <CrySandbox/CrySignal.h>

//////////////////////////////////////////////////////////////////////////
// General Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportGeneralPreferences : public SPreferencePage
{
	SViewportGeneralPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	float defaultFOV;
	float defaultAspectRatio;
	int   dragSquareSize;
	int   mapViewportResolution;

	bool  applyConfigSpec;
	bool  sync2DViews;
	bool  showSafeFrame;
	bool  hideMouseCursorWhenCaptured;
	bool  enableContextMenu;
	bool  toolsRenderUpdateMutualExclusive;
	bool  mapViewportSwapXY;
};

//////////////////////////////////////////////////////////////////////////
// Debug Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportDebugPreferences : public SPreferencePage
{
	SViewportDebugPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	void         SetObjectHideMask(int hideMask);
	int          GetObjectHideMask() const { return objectHideMask; }

	float              warningIconsDrawDistance;
	bool               showScaleWarnings;
	bool               showRotationWarnings;

	CCrySignal<void()> objectHideMaskChanged;

private:
	int objectHideMask;
};

//////////////////////////////////////////////////////////////////////////
// Movement Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportMovementPreferences : public SPreferencePage
{
	enum EWheelBehavior
	{
		eWheel_ZoomOnly  = 0,
		eWheel_SpeedOnly = 1,
		eWheel_ZoomSpeed = 2,
	};

	SViewportMovementPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	float camMoveSpeed;
	float camRotateSpeed;
	float camFastMoveSpeed;
	float wheelZoomSpeed;
	int   mouseWheelBehavior;
};

//////////////////////////////////////////////////////////////////////////
// Selection Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportSelectionPreferences : public SPreferencePage
{
	SViewportSelectionPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	ColorB colorPrefabBBox;
	ColorB colorGroupBBox;
	ColorB colorEntityBBox;
	ColorB geometryHighlightColor;
	ColorB geometrySelectionColor;
	ColorB solidBrushGeometryColor;
	float  bboxAlpha;
	float  geomAlpha;
	float  childObjectGeomAlpha;
	int    objectSelectMask;
	int    outlinePixelWidth;
	float  outlineGhostAlpha;
};

EDITOR_COMMON_API extern SViewportGeneralPreferences gViewportPreferences;
EDITOR_COMMON_API extern SViewportDebugPreferences gViewportDebugPreferences;
EDITOR_COMMON_API extern SViewportMovementPreferences gViewportMovementPreferences;
EDITOR_COMMON_API extern SViewportSelectionPreferences gViewportSelectionPreferences;
