// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Skinned Render mesh
// Contains skinned character data. It can be any asset that is animated with bone - weighted vertices like humans, aliens, ropes, lamps, heads, and parachutes.The.skin file includes the mesh, vertex weighting, vertex colors, and morph targets.
class CSkinnedMeshType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CSkinnedMeshType);

	virtual char* GetTypeName() const { return "SkinnedMesh"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Skinned Mesh"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return true; }
	virtual CAssetEditor* Edit(CAsset* asset) const override;
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual std::vector<string> GetImportExtensions() const override;
	virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
	virtual std::vector<SDetail> GetDetails() const override;
};
