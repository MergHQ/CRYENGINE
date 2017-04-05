// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Material
// Contains settings for shaders, surface types, and references to textures. The .mtl file is a text file which holds all the information for the in - game material library.The material library is a collection of sub materials which can be assigned to each face of a geometry.You can have for example different surfaces like metal, plastic, humanskin within different IDs of the asset.Each of these sub materials can use different shaders and different textures.
class CMaterialType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CMaterialType);

	virtual char* GetTypeName() const { return "Material"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Material"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return false; }
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual std::vector<string> GetImportExtensions() const override;
	virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
};