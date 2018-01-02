// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemAssets.h"

namespace ACE
{
class CSystemAssetsManager;

class CFileLoader
{
public:

	CFileLoader(CSystemAssetsManager& assetsManager);
	std::set<string> GetLoadedFilenamesList();
	void             LoadAll();
	void             LoadControls();
	void             LoadScopes();
	EErrorCode       GetErrorCodeMask() const { return m_errorCodeMask; }

private:

	using SwitchStates = std::vector<char const*>;

	void            LoadAllLibrariesInFolder(string const& folderPath, string const& level);
	void            LoadControlsLibrary(XmlNodeRef const pRoot, string const& filepath, string const& level, string const& filename, uint32 const version);
	CSystemControl* LoadControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CSystemAsset* const pParentItem);

	void            LoadPreloadConnections(XmlNodeRef const pNode, CSystemControl* const pControl, uint32 const version);
	void            LoadConnections(XmlNodeRef const root, CSystemControl* const pControl);

	void            CreateDefaultControls();
	bool            CreateDefaultSwitch(CSystemAsset* const pLibrary, char const* const szExternalName, char const* const szInternalName, SwitchStates const& states);

	void            LoadScopesImpl(string const& path);

	void            LoadEditorData(XmlNodeRef const pEditorDataNode, CSystemAsset& library);
	void            LoadLibraryEditorData(XmlNodeRef const pLibraryNode, CSystemAsset& library);
	void            LoadAllFolders(XmlNodeRef const pFoldersNode, CSystemAsset& library);
	void            LoadFolderData(XmlNodeRef const pFolderNode, CSystemAsset& parentAsset);
	void            LoadAllControlsEditorData(XmlNodeRef const pControlsNode);
	void            LoadControlsEditorData(XmlNodeRef const pParentNode);

	CSystemAsset*   AddUniqueFolderPath(CSystemAsset* pParent, QString const& path);

	CSystemAssetsManager& m_assetsManager;
	std::set<string>      m_loadedFilenames;
	EErrorCode            m_errorCodeMask;
};
} // namespace ACE
