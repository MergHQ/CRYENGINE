// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetType.h>

//! Prefabs are groups of objects that can be placed in the level as instances.
class CPrefabAssetType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CPrefabAssetType);

	virtual const char* GetTypeName() const                    { return "Prefab"; }
	virtual const char* GetUiTypeName() const                  { return "Prefab"; }
	virtual const char* GetFileExtension() const               { return "prefab"; }
	virtual QColor      GetThumbnailColor() const override     { return QColor(179, 179, 179); }
	virtual bool        CanBeCreated() const                   { return true; }
	virtual bool        CanBeCopied() const                    { return true; }
	virtual bool        CanBeEdited() const                    { return false; }
	virtual bool        CanAutoRepairMetadata() const override { return false; } //! Prefab assets use unique guids that can not be automatically restored.
	virtual const char* GetObjectClassName() const             { return "Prefab"; }
	virtual string      GetObjectFilePath(const CAsset* pAsset) const;
	virtual void        PreDeleteAssetFiles(const CAsset& asset) const override;
	virtual bool        RenameAsset(CAsset* pAsset, const char* szNewName) const override;

private:

	//! Initializes the asset. Creates all the necessary asset files.
	//! \param pCreateParams Points to an instance of SPrefabCreateParams or nullptr.
	//! \sa SPrefabCreateParams
	virtual bool    OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const override;
	virtual bool    OnCopy(INewAsset& asset, CAsset& assetToCopy) const override;
	virtual CryIcon GetIconInternal() const override;
};
