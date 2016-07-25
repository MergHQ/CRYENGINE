// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AudioWwiseLoader.h"
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/ILocalizationManager.h>
#include <CryString/CryPath.h>
#include "IAudioSystemEditor.h"
#include <IAudioSystemItem.h>
#include "AudioSystemEditor_wwise.h"

using namespace PathUtil;

namespace ACE
{
const string g_gameParametersFolder = "Game Parameters";
const string g_gameStatesPath = "States";
const string g_switchesFolder = "Switches";
const string g_eventsFolder = "Events";
const string g_environmentsFolder = "Master-Mixer Hierarchy";
const string g_soundBanksPath = AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "wwise";
const string g_soundBanksInfoFilename = "SoundbanksInfo.xml";

EWwiseItemTypes TagToItemType(const string& tag)
{
	if (tag == "GameParameter")
	{
		return eWwiseItemTypes_Rtpc;
	}
	else if (tag == "Event")
	{
		return eWwiseItemTypes_Event;
	}
	else if (tag == "AuxBus")
	{
		return eWwiseItemTypes_AuxBus;
	}
	else if (tag == "WorkUnit")
	{
		return eWwiseItemTypes_WorkUnit;
	}
	else if (tag == "StateGroup")
	{
		return eWwiseItemTypes_StateGroup;
	}
	else if (tag == "SwitchGroup")
	{
		return eWwiseItemTypes_SwitchGroup;
	}
	else if (tag == "Switch")
	{
		return eWwiseItemTypes_Switch;
	}
	else if (tag == "State")
	{
		return eWwiseItemTypes_State;
	}
	else if (tag == "Folder")
	{
		return eWwiseItemTypes_VirtualFolder;
	}
	return eWwiseItemTypes_Invalid;
}

CAudioWwiseLoader::CAudioWwiseLoader(const string& projectPath, const string& soundbanksPath, IAudioSystemItem& root)
	: m_root(root)
	, m_projectRoot(projectPath)
{
	LoadEventsMetadata(soundbanksPath);

	LoadFolder(g_gameParametersFolder, *CreateItem("Game Parameters", eWwiseItemTypes_PhysicalFolder, m_root));
	LoadFolder(g_gameStatesPath, *CreateItem("States", eWwiseItemTypes_PhysicalFolder, m_root));
	LoadFolder(g_switchesFolder, *CreateItem("Switches", eWwiseItemTypes_PhysicalFolder, m_root));
	LoadFolder(g_eventsFolder, *CreateItem("Events", eWwiseItemTypes_PhysicalFolder, m_root));
	LoadFolder(g_environmentsFolder, *CreateItem("Master-Mixer Hierarchy", eWwiseItemTypes_PhysicalFolder, m_root));

	IAudioSystemItem* pSoundbanksItem = CreateItem("SoundBanks", eWwiseItemTypes_PhysicalFolder, m_root);
	LoadSoundBanks(soundbanksPath, false, *pSoundbanksItem);

	// We consider only the language that is currently set.
	// This should be adjusted once the concept of a global reference language exists.
	char const* const szLanguage = gEnv->pSystem->GetLocalizationManager()->GetLanguage();
	LoadSoundBanks(PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + szLanguage + CRY_NATIVE_PATH_SEPSTR + g_soundBanksPath, true, *pSoundbanksItem);

}

void CAudioWwiseLoader::LoadSoundBanks(const string& rootFolder, const bool bLocalized, IAudioSystemItem& parent)
{
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(rootFolder + CRY_NATIVE_PATH_SEPSTR + "*.*", &fd);
	if (handle != -1)
	{
		const string ignoreFilename = "Init.bnk";
		do
		{
			string name = fd.name;
			if (name != "." && name != ".." && !name.empty())
			{
				if (name.find(".bnk") != string::npos && name.compareNoCase(ignoreFilename) != 0)
				{
					string fullname = parent.GetName() + CRY_NATIVE_PATH_SEPSTR + name;
					if (bLocalized)
					{
						fullname = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + fullname;
					}

					const CID id = CCrc32::ComputeLowercase(fullname);

					IAudioSystemItem* pControl = stl::find_in_map(m_controlsCache, id, nullptr);
					if (pControl == nullptr)
					{
						IAudioSystemControl_wwise* pNewControl = new IAudioSystemControl_wwise(name, id, eWwiseItemTypes_SoundBank);
						parent.AddChild(pNewControl);
						pNewControl->SetParent(&parent);
						pNewControl->SetLocalised(bLocalized);
						m_controlsCache[id] = pNewControl;
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}
}

void CAudioWwiseLoader::LoadFolder(const string& folderPath, IAudioSystemItem& parent)
{
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(m_projectRoot + CRY_NATIVE_PATH_SEPSTR + folderPath + CRY_NATIVE_PATH_SEPSTR "*.*", &fd);
	if (handle != -1)
	{
		do
		{
			string name = fd.name;
			if (name != "." && name != ".." && !name.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					LoadFolder(folderPath + CRY_NATIVE_PATH_SEPSTR + name, *CreateItem(name, eWwiseItemTypes_PhysicalFolder, parent));
				}
				else
				{
					LoadFile(name, folderPath, parent);
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}
}

void CAudioWwiseLoader::LoadFile(const string& filename, const string& path, IAudioSystemItem& parent)
{
	XmlNodeRef pRoot = GetISystem()->LoadXmlFromFile(m_projectRoot + CRY_NATIVE_PATH_SEPSTR + path + CRY_NATIVE_PATH_SEPSTR + filename);
	if (pRoot)
	{
		LoadXml(pRoot, parent, path);
	}
}

void CAudioWwiseLoader::LoadXml(XmlNodeRef pRoot, IAudioSystemItem& parent, const string& filePath)
{
	if (pRoot)
	{
		IAudioSystemItem* pControl = &parent;
		string childFilePath = filePath;
		const EWwiseItemTypes type = TagToItemType(pRoot->getTag());
		if (type != eWwiseItemTypes_Invalid)
		{
			string name = pRoot->getAttr("Name");
			pControl = CreateItem(name, type, parent, filePath);
			childFilePath += CRY_NATIVE_PATH_SEPSTR + name;
		}

		const int childCount = pRoot->getChildCount();
		for (int i = 0; i < childCount; ++i)
		{
			LoadXml(pRoot->getChild(i), *pControl, childFilePath);
		}
	}
}

IAudioSystemItem* CAudioWwiseLoader::CreateItem(const string& name, ItemType type, IAudioSystemItem& parent, const string& path)
{
	const string fullPathName = path.empty() ? name : path + CRY_NATIVE_PATH_SEPSTR + name;

	// The id is always the path of the control from the root of the wwise project
	CID id = CCrc32::ComputeLowercase(fullPathName);

	IAudioSystemItem* pControl = stl::find_in_map(m_controlsCache, id, nullptr);
	if (pControl == nullptr)
	{
		pControl = new IAudioSystemControl_wwise(name, id, type);
		if (type == eWwiseItemTypes_Event)
		{
			pControl->SetRadius(m_eventsInfoMap[id].maxRadius);
		}
		parent.AddChild(pControl);
		pControl->SetParent(&parent);
		m_controlsCache[id] = pControl;
	}
	return pControl;
}

void CAudioWwiseLoader::LoadEventsMetadata(const string& soundbanksPath)
{
	m_eventsInfoMap.clear();
	string path = soundbanksPath;
	path += CRY_NATIVE_PATH_SEPSTR + g_soundBanksInfoFilename;
	XmlNodeRef pRootNode = GetISystem()->LoadXmlFromFile(path.c_str());
	if (pRootNode)
	{
		XmlNodeRef pSoundBanksNode = pRootNode->findChild("SoundBanks");
		if (pSoundBanksNode)
		{
			const int numSoundBankNodes = pSoundBanksNode->getChildCount();
			for (int i = 0; i < numSoundBankNodes; ++i)
			{
				XmlNodeRef pSoundBankNode = pSoundBanksNode->getChild(i);
				if (pSoundBankNode)
				{
					XmlNodeRef pIncludedEventsNode = pSoundBankNode->findChild("IncludedEvents");
					if (pIncludedEventsNode)
					{
						const int numEventNodes = pIncludedEventsNode->getChildCount();
						for (int j = 0; j < numEventNodes; ++j)
						{
							XmlNodeRef pEventNode = pIncludedEventsNode->getChild(j);
							if (pEventNode)
							{
								SEventInfo info;
								if (!pEventNode->getAttr("MaxAttenuation", info.maxRadius))
								{
									info.maxRadius = 0.0f;
								}
								m_eventsInfoMap[CCrc32::ComputeLowercase(pEventNode->getAttr("Name"))] = info;
							}
						}
					}
				}
			}
		}
	}
}

ACE::IAudioSystemItem* CAudioWwiseLoader::GetControlByName(const string& name, bool bIsLocalised, IAudioSystemItem* pParent) const
{
	string fullName = (pParent) ? pParent->GetName() + CRY_NATIVE_PATH_SEPSTR + name : name;
	if (bIsLocalised)
	{
		fullName = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + fullName;
	}

	return stl::find_in_map(m_controlsCache, CCrc32::ComputeLowercase(fullName), nullptr);
}

}
