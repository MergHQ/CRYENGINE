// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

// Skeleton
// Contains the skeleton data and physics proxies used for hit detection and ragdoll simulation which is driven by animations.
class CSkeletonType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CSkeletonType);

	virtual char* GetTypeName() const { return "Skeleton"; }
	virtual char* GetUiTypeName() const { return QT_TR_NOOP("Skeleton"); }
	virtual bool IsImported() const { return true; }
	virtual bool CanBeEdited() const { return true; }
	virtual CAssetEditor* Edit(CAsset* asset) const override;
	virtual string GetTargetFilename(const string& sourceFilename) const override;
	virtual std::vector<string> GetImportExtensions() const override;
	virtual CryIcon GetIcon() const override;
	virtual CAsset* Import(const string&, CAssetImportContext& context) const override;
	virtual std::vector<SDetail> GetDetails() const override;

public:
	//! May be referenced by other asset types, too.
	static CItemModelAttribute s_bonesCountAttribute;
};
