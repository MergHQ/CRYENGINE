// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AudioAssets.h"

#include <ACETypes.h>
#include <IAudioConnection.h>
#include <CryString/CryString.h>
#include <CrySystem/XML/IXml.h>
#include <CrySystem/ISystem.h>

namespace ACE
{
class CAudioAssetsManager;
class IAudioSystemEditor;

struct SLibraryScope
{
	SLibraryScope()
		: bDirty(false)
	{
		pNodes[0] = GetISystem()->CreateXmlNode("AudioTriggers");
		pNodes[1] = GetISystem()->CreateXmlNode("AudioRTPCs");
		pNodes[2] = GetISystem()->CreateXmlNode("AudioSwitches");
		pNodes[3] = GetISystem()->CreateXmlNode("AudioEnvironments");
		pNodes[4] = GetISystem()->CreateXmlNode("AudioPreloads");
	}

	XmlNodeRef GetXmlNode(EItemType const type) const
	{
		switch (type)
		{
		case EItemType::Trigger:
			return pNodes[0];
		case EItemType::Parameter:
			return pNodes[1];
		case EItemType::Switch:
			return pNodes[2];
		case EItemType::Environment:
			return pNodes[3];
		case EItemType::Preload:
			return pNodes[4];
		}
		return nullptr;
	}

	XmlNodeRef pNodes[5]; // Trigger, Parameter, Switch, Environment, Preloads
	bool       bDirty;
};

typedef std::map<Scope, SLibraryScope> LibraryStorage;

class CAudioControlsWriter
{
public:

	CAudioControlsWriter(CAudioAssetsManager* pAssetsManager, IAudioSystemEditor* pAudioSystemImpl, std::set<string>& previousLibraryPaths);

private:

	void WriteLibrary(CAudioLibrary& library);
	void WriteItem(CAudioAsset* pItem, const string& path, LibraryStorage& library);
	void GetScopes(CAudioAsset const* const pItem, std::unordered_set<Scope>& scopes);
	void WriteControlToXML(XmlNodeRef pNode, CAudioControl* pControl, const string& path);
	void WriteConnectionsToXML(XmlNodeRef pNode, CAudioControl* pControl, const int platformIndex = -1);
	void WriteEditorData(CAudioAsset* pLibrary, XmlNodeRef pParentNode) const;

	void CheckOutFile(string const& filepath);
	void DeleteLibraryFile(string const& filepath);

	CAudioAssetsManager* m_pAssetsManager;
	IAudioSystemEditor*  m_pAudioSystemImpl;

	std::set<string>     m_foundLibraryPaths;

	static string const  s_controlsPath;
	static string const  s_levelsFolder;
	static uint const    s_currentFileVersion;
};
} // namespace ACE
