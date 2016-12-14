// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "EditorFramework/Preferences.h"

enum EDebugSettingsFlags
{
	DBG_MEMINFO                        = 0x002,
	DBG_FRAMEPROFILE                   = 0x400,
	DBG_HIGHLIGHT_BREAKABLE            = 0x2000,
	DBG_HIGHLIGHT_MISSING_SURFACE_TYPE = 0x4000
};

//////////////////////////////////////////////////////////////////////////
// General Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportGeneralPreferences : public SPreferencePage
{
	SViewportGeneralPreferences();
	virtual bool Serialize(yasli::Archive& ar) override;

	bool  applyConfigSpec;
	bool  sync2DViews;
	float defaultFOV;
	float defaultAspectRatio;
	bool  showSafeFrame;
	bool  hideMouseCursorWhenCaptured;
	int   dragSquareSize;
	bool  enableContextMenu;

	bool  displayLabels;
	float labelsDistance;
	bool  displayHelpers;
	bool  displayTracks;
	bool  displayLinks;
	bool  alwaysShowRadiuses;
	bool  alwaysShowPrefabBox;
	bool  alwaysShowPrefabObjects;
	bool  showBBoxes;
	bool  drawEntityLabels;
	bool  showTriggerBounds;
	bool  showIcons;
	bool  distanceScaleIcons;
	bool  showSizeBasedIcons;
	bool  showFrozenHelpers;
	bool  fillSelectedShapes;
	bool  showGridGuide;
	bool  displayDimension;
	bool  displaySelectedObjectOrientation;

	bool  mapViewportSwapXY;
	int   mapViewportResolution;
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

	void         SetDebugFlags(int flags);
	int          GetDebugFlags() const { return debugFlags; }

	float warningIconsDrawDistance;
	bool  showMeshStatsOnMouseOver;
	bool  showScaleWarnings;
	bool  showRotationWarnings;

	CCrySignal<void()> objectHideMaskChanged;
	CCrySignal<void()> debugFlagsChanged;

private:
	int objectHideMask;
	int debugFlags;
};

//////////////////////////////////////////////////////////////////////////
// Movement Preferences
//////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API SViewportMovementPreferences : public SPreferencePage
{
	enum EWheelBehavior
	{
		eWheel_ZoomOnly = 0,
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

	bool   highlightSelGeom;
	bool   highlightSelVegetation;
	bool   highlightOnMouseOver;
};

EDITOR_COMMON_API extern SViewportGeneralPreferences gViewportPreferences;
EDITOR_COMMON_API extern SViewportDebugPreferences gViewportDebugPreferences;
EDITOR_COMMON_API extern SViewportMovementPreferences gViewportMovementPreferences;
EDITOR_COMMON_API extern SViewportSelectionPreferences gViewportSelectionPreferences;
