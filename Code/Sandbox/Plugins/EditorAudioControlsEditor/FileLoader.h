// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"

class XmlNodeRef;

namespace ACE
{
class CAsset;
class CControl;

class CFileLoader final
{
public:

	CFileLoader(CFileLoader const&) = delete;
	CFileLoader(CFileLoader&&) = delete;
	CFileLoader& operator=(CFileLoader const&) = delete;
	CFileLoader& operator=(CFileLoader&&) = delete;

	CFileLoader() = default;

	FileNames  GetLoadedFilenamesList() const { return m_loadedFilenames; }
	void       CreateInternalControls();
	void       LoadAll();
	void       LoadScopes();
	EErrorCode GetErrorCodeMask() const { return m_errorCodeMask; }

private:

	void      LoadControls();
	void      LoadAllLibrariesInFolder(string const& folderPath, string const& level);
	void      LoadControlsLibrary(XmlNodeRef const pRoot, string const& filepath, string const& level, string const& filename, uint32 const version);
	CControl* LoadControl(XmlNodeRef const pNode, Scope const scope, uint32 const version, CAsset* const pParentItem);
	void      LoadPlatformSpecificConnections(XmlNodeRef const pNode, CControl* const pControl, uint32 const version);

	FileNames  m_loadedFilenames;
	EErrorCode m_errorCodeMask = EErrorCode::None;
};
} // namespace ACE
