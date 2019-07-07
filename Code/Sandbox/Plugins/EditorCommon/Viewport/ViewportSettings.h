// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "EditorCommonAPI.h"
#include <CrySandbox/CrySignal.h>
#include <CrySerialization/Forward.h>
#include <QVariantMap>

namespace yasli
{
class Archive;
}

struct EDITOR_COMMON_API SViewportHelperSettings
{
	SViewportHelperSettings();

	// Serialization inside Layout: the place to keep per-viewport data
	QVariantMap GetState() const;
	void        SetState(const QVariantMap& state);

	// Serialization for property tree
	bool Serialize(yasli::Archive& ar);

	bool enabled;

	// Basic
	bool showIcons;
	bool showMesh;

	// Additional options
	bool showAnimationTracks;
	bool showBoundingBoxes;
	bool showComponentHelpers;
	bool showDimensions; // Of selected objects except EntityObjects
	bool fillSelectedShapes;
	bool showFrozenObjectsHelpers;
	bool showTextLabels; // All text labels BUT EntityObjects
	bool showEntityObjectsTextLabels;
	bool showLinks;
	bool showMeshStatsOnMouseOver;
	bool showRadii;
	bool showSelectedObjectOrientation;
	bool showSnappingGridGuide;
	bool showTriggerBounds;

	// Per Type
	bool               showAreaHelper;
	bool               showBrushHelper;
	bool               showDecalHelper;
	bool               showEntityObjectHelper;
	bool               showEnviromentProbeHelper;
	bool               showGroupHelper;
	bool               showPrefabHelper;
	bool               showPrefabBounds;
	bool               showPrefabChildrenHelpers;
	bool               showRoadHelper;
	bool               showShapeHelper;

	CCrySignal<void()> signalStateChanged;

private:
	// Load bool from Layout's data
	void LoadBool(const QVariantMap& state, const char* name, bool& value);

	// Don't store CVars, just read them from current state and change the CVars itself if user pressed CheckBox
	void SerializeGlobalCVarBool(Serialization::IArchive& ar, const char* cvar, const char* display, int valueOn = 1, int valueOff = 0);
};
