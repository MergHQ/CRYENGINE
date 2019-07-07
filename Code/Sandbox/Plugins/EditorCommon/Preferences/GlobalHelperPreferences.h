// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include <EditorFramework/Preferences.h>
#include <CrySandbox/CrySignal.h>

// These settings will apply to helpers in any opened viewport
struct EDITOR_COMMON_API SGlobalHelperPreferences : public SPreferencePage
{
	SGlobalHelperPreferences();

	virtual bool Serialize(yasli::Archive& ar) override;

	void SetShowDesignerVolumes(bool show);

	bool               distanceScaleIcons;
	float              objectIconsScaleThreshold;
	float              objectIconsScaleThresholdSquared;

	bool               bHideDistancedHelpers;
	float              objectHelperMaxDistDisplay;
	float              objectHelperMaxDistSquaredDisplay;

	float              textLabelDistance;

	float              selectionHelperDisplayThreshold; // No CheckBox control here
	float              selectionHelperDisplayThresholdSquared;

	bool               objectIconsOnTop;

	bool               highlightBreakableMaterial;
	bool               highlightMissingSurfaceTypes;

	bool               showDesignerVolumes;

	CCrySignal<void()> materialSettingsChanged;
	CCrySignal<void()> designerVolumesChanged;

private:
	void SerializeGeneralSection(yasli::Archive& ar);
	void SerializeMaterialsSection(yasli::Archive& ar);
	void SerializeDesignerVolumes(yasli::Archive& ar);
};

EDITOR_COMMON_API extern SGlobalHelperPreferences gGlobalHelperPreferences;
