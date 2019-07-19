// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AnimationType.h"

#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/AssetResourceSelector.h>

REGISTER_ASSET_TYPE(CAnimationType)

CAssetEditor* CAnimationType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Animation", asset);
}

CryIcon CAnimationType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_animation.ico");
}

//Group all animation-derived types into one practical "AnyAnimation" selector
REGISTER_RESOURCE_SELECTOR_ASSET_MULTIPLETYPES("AnyAnimation", std::vector<string>({ "Animation", "MeshAnimation" }));
