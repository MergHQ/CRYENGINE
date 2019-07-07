// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ViewportSettings.h"
#include <CrySystem/IConsole.h>

SViewportHelperSettings::SViewportHelperSettings()
	: enabled(true)
	, showIcons(true)
	, showMesh(true)
	, showAnimationTracks(true)
	, showBoundingBoxes(false)
	, showComponentHelpers(true)
	, showDimensions(false)
	, fillSelectedShapes(false)
	, showFrozenObjectsHelpers(true)
	, showTextLabels(false)
	, showEntityObjectsTextLabels(false)
	, showLinks(true)
	, showMeshStatsOnMouseOver(false)
	, showRadii(false)
	, showSelectedObjectOrientation(false)
	, showSnappingGridGuide(true)
	, showTriggerBounds(false)
	, showAreaHelper(true)
	, showBrushHelper(true)
	, showDecalHelper(true)
	, showEntityObjectHelper(true)
	, showEnviromentProbeHelper(true)
	, showGroupHelper(true)
	, showPrefabHelper(true)
	, showPrefabBounds(true)
	, showPrefabChildrenHelpers(true)
	, showRoadHelper(true)
	, showShapeHelper(true)
{}

QVariantMap SViewportHelperSettings::GetState() const
{
	QVariantMap state;

	state.insert("enabled", enabled);
	state.insert("showIcons", showIcons);
	state.insert("showMesh", showMesh);

	// Additional options
	state.insert("showAnimationTracks", showAnimationTracks);
	state.insert("showBoundingBoxes", showBoundingBoxes);
	state.insert("showComponentHelpers", showComponentHelpers);
	state.insert("showDimensions", showDimensions);
	state.insert("fillSelectedShapes", fillSelectedShapes);
	state.insert("showFrozenObjectsHelpers", showFrozenObjectsHelpers);
	state.insert("showTextLabels", showTextLabels);
	state.insert("showEntityObjectsTextLabels", showEntityObjectsTextLabels);
	state.insert("showLinks", showLinks);
	state.insert("showMeshStatsOnMouseOver", showMeshStatsOnMouseOver);
	state.insert("showRadii", showRadii);
	state.insert("showSelectedObjectOrientation", showSelectedObjectOrientation);
	state.insert("showSnappingGridGuide", showSnappingGridGuide);
	state.insert("showTriggerBounds", showTriggerBounds);

	// Per Type
	state.insert("showAreaHelper", showAreaHelper);
	state.insert("showBrushHelper", showBrushHelper);
	state.insert("showDecalHelper", showDecalHelper);
	state.insert("showEntityObjectHelper", showEntityObjectHelper);
	state.insert("showEnviromentProbeHelper", showEnviromentProbeHelper);
	state.insert("showGroupHelper", showGroupHelper);
	state.insert("showPrefabHelper", showPrefabHelper);
	state.insert("showPrefabBounds", showPrefabBounds);
	state.insert("showPrefabChildrenHelpers", showPrefabChildrenHelpers);
	state.insert("showRoadHelper", showRoadHelper);
	state.insert("showShapeHelper", showShapeHelper);

	return state;
}

void SViewportHelperSettings::SetState(const QVariantMap& state)
{
	LoadBool(state, "enabled", enabled);
	LoadBool(state, "showIcons", showIcons);
	LoadBool(state, "showMesh", showMesh);

	// Additional options
	LoadBool(state, "showAnimationTracks", showAnimationTracks);
	LoadBool(state, "showBoundingBoxes", showBoundingBoxes);
	LoadBool(state, "showComponentHelpers", showComponentHelpers);
	LoadBool(state, "showDimensions", showDimensions);
	LoadBool(state, "fillSelectedShapes", fillSelectedShapes);
	LoadBool(state, "showFrozenObjectsHelpers", showFrozenObjectsHelpers);
	LoadBool(state, "showTextLabels", showTextLabels);
	LoadBool(state, "showEntityObjectsTextLabels", showEntityObjectsTextLabels);
	LoadBool(state, "showLinks", showLinks);
	LoadBool(state, "showMeshStatsOnMouseOver", showMeshStatsOnMouseOver);
	LoadBool(state, "showRadii", showRadii);
	LoadBool(state, "showSelectedObjectOrientation", showSelectedObjectOrientation);
	LoadBool(state, "showSnappingGridGuide", showSnappingGridGuide);
	LoadBool(state, "showTriggerBounds", showTriggerBounds);

	// Per Type
	LoadBool(state, "showAreaHelper", showAreaHelper);
	LoadBool(state, "showBrushHelper", showBrushHelper);
	LoadBool(state, "showDecalHelper", showDecalHelper);
	LoadBool(state, "showEntityObjectHelper", showEntityObjectHelper);
	LoadBool(state, "showEnviromentProbeHelper", showEnviromentProbeHelper);
	LoadBool(state, "showGroupHelper", showGroupHelper);
	LoadBool(state, "showPrefabHelper", showPrefabHelper);
	LoadBool(state, "showPrefabBounds", showPrefabBounds);
	LoadBool(state, "showPrefabChildrenHelpers", showPrefabChildrenHelpers);
	LoadBool(state, "showRoadHelper", showRoadHelper);
	LoadBool(state, "showShapeHelper", showShapeHelper);

	signalStateChanged();
}

void SViewportHelperSettings::LoadBool(const QVariantMap& state, const char* name, bool& value)
{
	QVariant var = state.value(name);
	if (var.isValid() && var.type() == QVariant::Bool)
	{
		value = var.toBool();
	}
}

void SViewportHelperSettings::SerializeGlobalCVarBool(Serialization::IArchive& ar, const char* cvar, const char* display, int valueOn /*= 1*/, int valueOff /* = 0*/)
{
	// Get/Set state of global CVars only for property tree: they will not be stored between Sandbox runs
	ICVar* pVar = gEnv->pConsole->GetCVar(cvar);
	if (!pVar)
	{
		return;
	}

	bool b = pVar->GetIVal() == valueOn;
	bool prev = b;
	ar(b, cvar, display);
	if (ar.isInput() && b != prev)
	{
		pVar->Set(b ? valueOn : valueOff);
	}
}

bool SViewportHelperSettings::Serialize(yasli::Archive& ar)
{
	ar.openBlock("basic", "Basic");
	ar(showIcons, "showIcons", "Icon");
	ar(showMesh, "showMesh", "Mesh");
	SerializeGlobalCVarBool(ar, "p_draw_helpers", "Physics Proxies (*)");
	ar.closeBlock();

	ar.openBlock("additionalOptions", "Additional Options");
	SerializeGlobalCVarBool(ar, "ai_DebugDraw", "AI Debug Info (*)", 1, -1);
	SerializeGlobalCVarBool(ar, "ai_DebugDrawCover", "AI Draw Cover (*)", 2, 0);
	ar(showAnimationTracks, "animationTracks", "Animation Tracks");
	ar(showBoundingBoxes, "boundingBoxes", "Bounding Boxes");
	ar(showComponentHelpers, "componentHelpers", "Component Helpers");
	ar(showDimensions, "dimensions", "Dimensions");
	ar(fillSelectedShapes, "fillSelectedShapes", "Fill Selected Shapes");
	ar(showFrozenObjectsHelpers, "frozenObjectsHelpers", "Frozen Objects Helpers");
	SerializeGlobalCVarBool(ar, "gt_show", "Game Tokens (*)");
	ar(showTextLabels, "textLabels", "Labels");
	ar(showEntityObjectsTextLabels, "entityObjectsTextLabels", "Labels (Entities only)");
	ar(showLinks, "links", "Links");
	ar(showMeshStatsOnMouseOver, "meshStatistics", "Mesh Statistics (on hover)");
	ar(showPrefabBounds, "prefabBounds", "Prefab Bounds");
	ar(showRadii, "radii", "Radii");
	ar(showSnappingGridGuide, "snappingGridGuide", "Snapping Grid Guide");
	ar(showSelectedObjectOrientation, "selectedObjectOrientation", "Selected Objects orientation");
	ar(showTriggerBounds, "triggerBounds", "Trigger Bounds");
	ar.closeBlock();

	ar.openBlock("", "Helper per Object Type");
	ar(showAreaHelper, "areas", "Areas");
	ar(showBrushHelper, "brushes", "Brushes");
	ar(showDecalHelper, "decals", "Decals");
	ar(showEntityObjectHelper, "entityObjects", "Entity Objects");
	ar(showEnviromentProbeHelper, "environmentProbes", "Environment Probes");
	ar(showGroupHelper, "groups", "Groups");
	ar(showPrefabHelper, "prefabs", "Prefabs");
	// Spaces in the name are for make this item "child-like" from "Prefab" in the property tree
	ar(showPrefabChildrenHelpers, "prefabChildren", showPrefabHelper ? "       Prefab Children" : "!       Prefab Children");
	ar(showRoadHelper, "roads", "Roads");
	ar(showShapeHelper, "shapes", "Shapes");
	ar.closeBlock();

	return true;
}
