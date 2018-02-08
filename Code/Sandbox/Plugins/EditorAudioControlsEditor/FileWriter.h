// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemAssets.h"

#include <CryAudio/IAudioSystem.h>

namespace ACE
{
class CSystemAssetsManager;
struct IEditorImpl;

static constexpr char* s_szLibraryNodeTag = "Library";
static constexpr char* s_szFoldersNodeTag = "Folders";
static constexpr char* s_szControlsNodeTag = "Controls";
static constexpr char* s_szFolderTag = "Folder";
static constexpr char* s_szPathAttribute = "path";
static constexpr char* s_szDescriptionAttribute = "description";

struct SLibraryScope
{
	SLibraryScope()
		: isDirty(false)
	{
		pNodes[0] = GetISystem()->CreateXmlNode(CryAudio::s_szTriggersNodeTag);
		pNodes[1] = GetISystem()->CreateXmlNode(CryAudio::s_szParametersNodeTag);
		pNodes[2] = GetISystem()->CreateXmlNode(CryAudio::s_szSwitchesNodeTag);
		pNodes[3] = GetISystem()->CreateXmlNode(CryAudio::s_szEnvironmentsNodeTag);
		pNodes[4] = GetISystem()->CreateXmlNode(CryAudio::s_szPreloadsNodeTag);
	}

	XmlNodeRef GetXmlNode(ESystemItemType const type) const
	{
		XmlNodeRef pNode;

		switch (type)
		{
		case ESystemItemType::Trigger:
			pNode = pNodes[0];
			break;
		case ESystemItemType::Parameter:
			pNode = pNodes[1];
			break;
		case ESystemItemType::Switch:
			pNode = pNodes[2];
			break;
		case ESystemItemType::Environment:
			pNode = pNodes[3];
			break;
		case ESystemItemType::Preload:
			pNode = pNodes[4];
			break;
		default:
			pNode = nullptr;
			break;
		}

		return pNode;
	}

	XmlNodeRef pNodes[5]; // Trigger, Parameter, Switch, Environment, Preload
	bool       isDirty;
};

using LibraryStorage = std::map<Scope, SLibraryScope>;

class CFileWriter
{
public:

	CFileWriter(CSystemAssetsManager const& assetsManager, std::set<string>& previousLibraryPaths);

	void WriteAll();

private:

	void WriteLibrary(CSystemLibrary const& library);
	void WriteItem(CSystemAsset* const pItem, string const& path, LibraryStorage& library);
	void GetScopes(CSystemAsset const* const pItem, std::unordered_set<Scope>& scopes);
	void WriteControlToXML(XmlNodeRef const pNode, CSystemControl* const pControl, string const& path);
	void WriteConnectionsToXML(XmlNodeRef const pNode, CSystemControl* const pControl, int const platformIndex = -1);
	void WriteLibraryEditorData(CSystemAsset const& library, XmlNodeRef const pParentNode) const;
	void WriteFolderEditorData(CSystemAsset const& library, XmlNodeRef const pParentNode) const;
	void WriteControlsEditorData(CSystemAsset const& parentAsset, XmlNodeRef const pParentNode) const;
	void DeleteLibraryFile(string const& filepath);

	CSystemAssetsManager const& m_assetsManager;

	std::set<string>&           m_previousLibraryPaths;
	std::set<string>            m_foundLibraryPaths;

	static uint32 const         s_currentFileVersion;
};
} // namespace ACE
