// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GlobalHelperPreferences.h"

#include <CrySerialization/yasli/decorators/Range.h>

EDITOR_COMMON_API SGlobalHelperPreferences gGlobalHelperPreferences;
REGISTER_PREFERENCES_PAGE_PTR(SGlobalHelperPreferences, &gGlobalHelperPreferences)

SGlobalHelperPreferences::SGlobalHelperPreferences()
	: SPreferencePage("Helpers", "Viewport/Helpers")
	, bHideDistancedHelpers(true)
	, objectHelperMaxDistDisplay(70)
	, objectHelperMaxDistSquaredDisplay(objectHelperMaxDistDisplay * objectHelperMaxDistDisplay)
	, textLabelDistance(100.0f)
	, distanceScaleIcons(true)
	, objectIconsScaleThreshold(50)
	, objectIconsScaleThresholdSquared(objectIconsScaleThreshold * objectIconsScaleThreshold)
	, selectionHelperDisplayThreshold(30)
	, selectionHelperDisplayThresholdSquared(selectionHelperDisplayThreshold * selectionHelperDisplayThreshold)
	, objectIconsOnTop(true)
	, highlightBreakableMaterial(false)
	, highlightMissingSurfaceTypes(false)
	, showDesignerVolumes(true)
{
}

bool SGlobalHelperPreferences::Serialize(yasli::Archive& ar)
{
	SerializeGeneralSection(ar);
	SerializeMaterialsSection(ar);
	SerializeDesignerVolumes(ar);

	return true;
}

void SGlobalHelperPreferences::SetShowDesignerVolumes(bool show)
{
	showDesignerVolumes = show;
	designerVolumesChanged();
}

void SGlobalHelperPreferences::SerializeGeneralSection(yasli::Archive& ar)
{
	ar.openBlock("general", "General");
	ar(distanceScaleIcons, "shrinkHelpersInDistance", "Shrink Helpers in Distance");

	//Empty string by design, no visibility in GUI if CheckBox is off
	ar(yasli::Range(objectIconsScaleThreshold, 0.0f, 500.0f), "objectIconsScaleThreshold", distanceScaleIcons ? " " : "");

	ar(bHideDistancedHelpers, "hideHelpersInDistance", "Hide Helpers in Distance");
	//Empty string by design, no visibility in GUI if CheckBox is off
	ar(yasli::Range(objectHelperMaxDistDisplay, 0.0f, 500.0f), "objectHelperMaxDistDisplay", bHideDistancedHelpers ? " " : "");

	ar(yasli::Range(textLabelDistance, 0.0f, 1000.0f), "textLabelDistance", "Hide Labels Distance");

	ar(yasli::Range(selectionHelperDisplayThreshold, 0.0f, 500.0f), "selectionHelperDisplayThreshold", "Hide Selection Helpers in Distance");

	ar(objectIconsOnTop, "renderIconsOnTopOf3dObjects", "Render Icons on Top of 3D Objects");

	ar.closeBlock();

	// Aux precomputed values
	if (ar.isInput())
	{
		objectIconsScaleThresholdSquared = objectIconsScaleThreshold * objectIconsScaleThreshold;
		objectHelperMaxDistSquaredDisplay = objectHelperMaxDistDisplay * objectHelperMaxDistDisplay;
		selectionHelperDisplayThresholdSquared = selectionHelperDisplayThreshold * selectionHelperDisplayThreshold;
	}
}

void SGlobalHelperPreferences::SerializeMaterialsSection(yasli::Archive& ar)
{
	const bool prevHighlightBreakableMaterial = highlightBreakableMaterial;
	const bool prevHighlightMissingSurfaceTypes = highlightMissingSurfaceTypes;

	ar.openBlock("materials", "Highlight Materials");
	ar(highlightBreakableMaterial, "highlightBreakableMaterial", "Breakable");
	ar(highlightMissingSurfaceTypes, "highlightMissingSurfaceTypes", "Missing Surface Types");
	ar.closeBlock();

	if (ar.isInput())
	{
		if (prevHighlightBreakableMaterial != highlightBreakableMaterial ||
		    prevHighlightMissingSurfaceTypes != prevHighlightMissingSurfaceTypes)
		{
			materialSettingsChanged();
		}
	}
}

void SGlobalHelperPreferences::SerializeDesignerVolumes(yasli::Archive& ar)
{
	const bool prevShowDesignerVolumes = showDesignerVolumes;
	ar.openBlock("designer", "Designer Volumes");
	ar(showDesignerVolumes, "showDesignerVolumes", "Area and Clip Volumes");
	ar.closeBlock();

	if (ar.isInput())
	{
		if (prevShowDesignerVolumes != showDesignerVolumes)
		{
			designerVolumesChanged();
		}
	}
}
