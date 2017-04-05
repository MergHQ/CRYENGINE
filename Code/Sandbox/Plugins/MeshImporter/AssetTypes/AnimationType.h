// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Character Animation
// Compressed representation of the i_caf format. This format uses lossy compression and such files are created either by Sandbox or during the asset build build.Such files are loaded by the engine at runtime.
class CAnimationType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CAnimationType);

	virtual char* GetTypeName() const { return "Animation"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Animation"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return true; }
	virtual CAssetEditor* Edit(CAsset* asset) const override;
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual std::vector<string> GetImportExtensions() const override;
		virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
};
