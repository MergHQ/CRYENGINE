// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Control.h"

#include <CrySystem/ISystem.h>

namespace ACE
{
class CLibrary;

static constexpr char const* s_szLibraryNodeTag = "Library";
static constexpr char const* s_szFoldersNodeTag = "Folders";
static constexpr char const* s_szControlsNodeTag = "Controls";
static constexpr char const* s_szFolderTag = "Folder";
static constexpr char const* s_szPathAttribute = "path";
static constexpr char const* s_szDescriptionAttribute = "description";

struct SLibraryScope final
{
	SLibraryScope()
	{
		pNodes[0] = GetISystem()->CreateXmlNode(CryAudio::s_szTriggersNodeTag);
		pNodes[1] = GetISystem()->CreateXmlNode(CryAudio::s_szParametersNodeTag);
		pNodes[2] = GetISystem()->CreateXmlNode(CryAudio::s_szSwitchesNodeTag);
		pNodes[3] = GetISystem()->CreateXmlNode(CryAudio::s_szEnvironmentsNodeTag);
		pNodes[4] = GetISystem()->CreateXmlNode(CryAudio::s_szPreloadsNodeTag);
		pNodes[5] = GetISystem()->CreateXmlNode(CryAudio::s_szSettingsNodeTag);
	}

	XmlNodeRef GetXmlNode(EAssetType const type) const
	{
		XmlNodeRef pNode;

		switch (type)
		{
		case EAssetType::Trigger:
			pNode = pNodes[0];
			break;
		case EAssetType::Parameter:
			pNode = pNodes[1];
			break;
		case EAssetType::Switch:
			pNode = pNodes[2];
			break;
		case EAssetType::Environment:
			pNode = pNodes[3];
			break;
		case EAssetType::Preload:
			pNode = pNodes[4];
			break;
		case EAssetType::Setting:
			pNode = pNodes[5];
			break;
		default:
			pNode = nullptr;
			break;
		}

		return pNode;
	}

	XmlNodeRef pNodes[6]; // Trigger, Parameter, Switch, Environment, Preload, Setting
	bool       isDirty = false;
	uint32     numTriggers = 0;
	uint32     numParameters = 0;
	uint32     numSwitches = 0;
	uint32     numStates = 0;
	uint32     numEnvironments = 0;
	uint32     numPreloads = 0;
	uint32     numSettings = 0;
	uint32     numTriggerConnections = 0;
	uint32     numParameterConnections = 0;
	uint32     numStateConnections = 0;
	uint32     numEnvironmentConnections = 0;
	uint32     numPreloadConnections = 0;
	uint32     numSettingConnections = 0;
};

using LibraryStorage = std::map<Scope, SLibraryScope>;

class CFileWriter final
{
public:

	explicit CFileWriter(FileNames& previousLibraryPaths);

	CFileWriter() = delete;

	void WriteAll();

private:

	void WriteLibrary(CLibrary& library);
	void WriteItem(CAsset* const pAsset, string const& path, LibraryStorage& library);
	void GetScopes(CAsset const* const pAsset, std::unordered_set<Scope>& scopes);
	void WriteControlToXML(XmlNodeRef const pNode, CControl* const pControl, string const& path, SLibraryScope& scope);
	void WriteConnectionsToXML(XmlNodeRef const pNode, CControl* const pControl, SLibraryScope& scope, int const platformIndex = -1);
	void WriteLibraryEditorData(CAsset const& library, XmlNodeRef const pParentNode) const;
	void WriteFolderEditorData(CAsset const& library, XmlNodeRef const pParentNode) const;
	void WriteControlsEditorData(CAsset const& parentAsset, XmlNodeRef const pParentNode) const;
	void DeleteLibraryFile(string const& filepath);

	FileNames&          m_previousLibraryPaths;
	FileNames           m_foundLibraryPaths;

	static uint32 const s_currentFileVersion;
};
} // namespace ACE
