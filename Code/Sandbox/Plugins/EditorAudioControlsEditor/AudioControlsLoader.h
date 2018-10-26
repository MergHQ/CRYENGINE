// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Control.h"

namespace ACE
{
// This file is deprecated and only used for backwards compatibility. It will be removed before March 2019.
class CAudioControlsLoader final
{
public:

	CAudioControlsLoader(CAudioControlsLoader const&) = delete;
	CAudioControlsLoader(CAudioControlsLoader&&) = delete;
	CAudioControlsLoader& operator=(CAudioControlsLoader const&) = delete;
	CAudioControlsLoader& operator=(CAudioControlsLoader&&) = delete;

	CAudioControlsLoader() = default;

	FileNames  GetLoadedFilenamesList();
	void       LoadAll(bool const loadOnlyDefaultControls = false);
	void       LoadControls(string const& folderPath);
	void       LoadScopes();
	EErrorCode GetErrorCodeMask() const { return m_errorCodeMask; }

private:

	using SwitchStates = std::vector<char const*>;

	void      LoadAllLibrariesInFolder(string const& folderPath, string const& level);
	void      LoadControlsLibrary(XmlNodeRef const pRoot, string const& filepath, string const& level, string const& filename, uint32 const version);
	CControl* LoadControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CAsset* const pParentItem);
	CControl* LoadDefaultControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CAsset* const pParentItem);

	void      LoadPreloadConnections(XmlNodeRef const pNode, CControl* const pControl, uint32 const version);
	void      LoadConnections(XmlNodeRef const root, CControl* const pControl);

	void      LoadScopesImpl(string const& path);

	void      LoadEditorData(XmlNodeRef const pEditorDataNode, CAsset& library);
	void      LoadLibraryEditorData(XmlNodeRef const pLibraryNode, CAsset& library);
	void      LoadAllFolders(XmlNodeRef const pFoldersNode, CAsset& library);
	void      LoadFolderData(XmlNodeRef const pFolderNode, CAsset& parentAsset);
	void      LoadAllControlsEditorData(XmlNodeRef const pControlsNode);
	void      LoadControlsEditorData(XmlNodeRef const pParentNode);

	CAsset*   AddUniqueFolderPath(CAsset* pParent, QString const& path);

	static string const s_controlsLevelsFolder;
	static string const s_assetsFolderPath;
	FileNames           m_loadedFilenames;
	EErrorCode          m_errorCodeMask = EErrorCode::None;
	bool                m_loadOnlyDefaultControls = false;

	FileNames           m_defaultTriggerNames { CryAudio::s_szGetFocusTriggerName, CryAudio::s_szLoseFocusTriggerName, CryAudio::s_szMuteAllTriggerName, CryAudio::s_szUnmuteAllTriggerName };
	FileNames           m_defaultParameterNames { "absolute_velocity", "object_speed", "relative_velocity", "object_doppler" };
};
} // namespace ACE
