// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Mesh
// Contain geometry data(grouped triangles, vertex attributes like tangent space or vertex color, optional physics data).
class CMeshType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CMeshType);

	virtual char* GetTypeName() const { return "Mesh"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Mesh"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return true; }
	virtual CAssetEditor* Edit(CAsset* asset) const override;
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual std::vector<string> GetImportExtensions() const override;
	virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
	virtual const char* GetObjectClassName() const { return "Brush"; }
	virtual std::vector<SDetail> GetDetails() const override;

public:
	//! May be referenced by other asset types, too.
	static CItemModelAttribute s_vertexCountAttribute;
	static CItemModelAttribute s_triangleCountAttribute;
	static CItemModelAttribute s_materialCountAttribute;
};

