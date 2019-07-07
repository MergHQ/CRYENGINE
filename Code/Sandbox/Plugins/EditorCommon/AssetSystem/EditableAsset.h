// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Asset.h"
#include "AssetType.h"
#include "EditorCommonAPI.h"

namespace AssetLoader
{
	struct SAssetSerializer;
	class CAssetFactory;
}

//! CEditableAsset modifies an asset.
//! Modification of assets is restricted to a small number of friend classes of CEditableAsset.
//! Everybody else is supposed to use the read-only CAsset interface.
class EDITOR_COMMON_API CEditableAsset : public IEditableAsset
{
	friend class CAsset;
	friend class CAssetType;
	friend class CAssetBrowser;
	friend class CAssetEditor;
	friend class CAssetManager;
	friend class CAssetImportContext;
	friend class AssetLoader::CAssetFactory;
	friend struct AssetLoader::SAssetSerializer;

	// TODO: Remove when CLevelEditor is CAssetEditor.
	friend class CLevelEditor;

public:
	CEditableAsset(const CEditableAsset&);
	CEditableAsset& operator=(const CEditableAsset&) = delete;

	CAsset& GetAsset();

	virtual const char* GetMetadataFile() const override;
	virtual const char* GetFolder() const override;
	virtual const char* GetName() const override;

	virtual void SetSourceFile(const char* szFilepath) override;
	virtual void AddFile(const char* szFilepath) override;
	virtual void SetFiles(const std::vector<string>& filenames) override;
	virtual void AddWorkFile(const char* szFilepath) override;
	virtual void SetWorkFiles(const std::vector<string>& filenames) override;
	virtual void SetDetails(const std::vector<std::pair<string, string>>& details) override;
	virtual void SetDependencies(const std::vector<SAssetDependencyInfo>& dependencies) override;

	bool WriteToFile();
	void SetName(const char* szName);
	void SetLastModifiedTime(uint64 t);
	void SetMetadataFile(const char* szFilepath);

	//! Invalidates thumbnail, so that subsequent calls to CAsset::StartLoadingThumbnail() will re-load
	//! the thumbnail from disk.
	void InvalidateThumbnail();

	void SetOpenedInAssetEditor(CAssetEditor* pEditor);

private:
	explicit CEditableAsset(CAsset& asset);

private:
	CAsset& m_asset;
};
