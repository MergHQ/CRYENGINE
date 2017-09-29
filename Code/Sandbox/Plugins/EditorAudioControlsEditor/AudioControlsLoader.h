// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioAssets.h"

namespace ACE
{
class CAudioAssetsManager;

class CAudioControlsLoader
{
public:

	CAudioControlsLoader(CAudioAssetsManager* pAssetsManager);
	std::set<string> GetLoadedFilenamesList();
	void             LoadAll();
	void             LoadControls();
	void             LoadScopes();
	EErrorCode       GetErrorCodeMask() const { return m_errorCodeMask; }

private:

	using SwitchStates = std::vector<const char*>;

	void           LoadAllLibrariesInFolder(string const& folderPath, string const& level);
	void           LoadControlsLibrary(XmlNodeRef const pRoot, string const& filepath, string const& level, string const& filename, uint const version);
	CAudioControl* LoadControl(XmlNodeRef const pNode, Scope scope, uint version, CAudioAsset* const pParentItem);

	void           LoadPreloadConnections(XmlNodeRef const pNode, CAudioControl* const pControl, uint const version);
	void           LoadConnections(XmlNodeRef const root, CAudioControl* const pControl);

	void           CreateDefaultControls();
	void           CreateDefaultSwitch(CAudioAsset* const pLibrary, char const* const szExternalName, char const* const szInternalName, SwitchStates const& states);

	void           LoadScopesImpl(string const& path);

	void           LoadEditorData(XmlNodeRef const pEditorDataNode, CAudioAsset* const pRootItem);
	void           LoadAllFolders(XmlNodeRef const pRootFoldersNode, CAudioAsset* const pParentItem);
	void           LoadFolderData(XmlNodeRef const pRootFoldersNode, CAudioAsset* const pParentItem);

	CAudioAsset*   AddUniqueFolderPath(CAudioAsset* pParent, QString const& path);

	static string const s_controlsLevelsFolder;
	static string const s_levelsFolder;
	// TODO: Move these strings to Utils

	CAudioAssetsManager* m_pAssetsManager;
	std::set<string>     m_loadedFilenames;
	EErrorCode           m_errorCodeMask;
};
} // namespace ACE
