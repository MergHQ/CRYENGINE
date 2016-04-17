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
const string g_gameParametersFolder = CRY_NATIVE_PATH_SEPSTR "Game Parameters";
const string g_gameStatesPath = CRY_NATIVE_PATH_SEPSTR "States";
const string g_switchesFolder = CRY_NATIVE_PATH_SEPSTR "Switches";
const string g_eventsFolder = CRY_NATIVE_PATH_SEPSTR  "Events";
const string g_environmentsFolder = CRY_NATIVE_PATH_SEPSTR "Master-Mixer Hierarchy";
const string g_soundBanksPath = AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "wwise";

void CAudioWwiseLoader::Load(CAudioSystemEditor_wwise* pAudioSystemImpl)
{
	m_pAudioSystemImpl = pAudioSystemImpl;
	string projectPath = pAudioSystemImpl->GetSettings()->GetProjectPath();
	LoadControlsInFolder(projectPath + g_gameParametersFolder);
	LoadControlsInFolder(projectPath + g_gameStatesPath);
	LoadControlsInFolder(projectPath + g_switchesFolder);
	LoadControlsInFolder(projectPath + g_eventsFolder);
	LoadControlsInFolder(projectPath + g_environmentsFolder);
	LoadSoundBanks(pAudioSystemImpl->GetSettings()->GetSoundBanksPath(), false);

	// We consider only the language that is currently set.
	// This should be adjusted once the concept of a global reference language exists.
	char const* const szLanguage = gEnv->pSystem->GetLocalizationManager()->GetLanguage();
	LoadSoundBanks(PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + szLanguage + CRY_NATIVE_PATH_SEPSTR + g_soundBanksPath, true);
}

void CAudioWwiseLoader::LoadSoundBanks(const string& rootFolder, const bool bLocalized)
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
					m_pAudioSystemImpl->CreateControl(SControlDef(name, eWwiseItemTypes_SoundBank, bLocalized, nullptr));
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}
}

void CAudioWwiseLoader::LoadControlsInFolder(const string& folderPath)
{
	_finddata_t fd;
	ICryPak* pCryPak = gEnv->pCryPak;
	intptr_t handle = pCryPak->FindFirst(folderPath + CRY_NATIVE_PATH_SEPSTR "*.*", &fd);
	if (handle != -1)
	{
		do
		{
			string name = fd.name;
			if (name != "." && name != ".." && !name.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					LoadControlsInFolder(folderPath + CRY_NATIVE_PATH_SEPSTR + name);
				}
				else
				{
					string filename = folderPath + CRY_NATIVE_PATH_SEPSTR + name;
					XmlNodeRef pRoot = GetISystem()->LoadXmlFromFile(filename);
					if (pRoot)
					{
						LoadControl(pRoot);
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);
		pCryPak->FindClose(handle);
	}
}

void CAudioWwiseLoader::ExtractControlsFromXML(XmlNodeRef root, EWwiseItemTypes type, const string& controlTag, const string& controlNameAttribute)
{
	string xmlTag = root->getTag();
	if (xmlTag.compare(controlTag) == 0)
	{
		string name = root->getAttr(controlNameAttribute);
		m_pAudioSystemImpl->CreateControl(SControlDef(name, type));
	}
}

void CAudioWwiseLoader::LoadControl(XmlNodeRef pRoot)
{
	if (pRoot)
	{
		ExtractControlsFromXML(pRoot, eWwiseItemTypes_Rtpc, "GameParameter", "Name");
		ExtractControlsFromXML(pRoot, eWwiseItemTypes_Event, "Event", "Name");
		ExtractControlsFromXML(pRoot, eWwiseItemTypes_AuxBus, "AuxBus", "Name");

		// special case for switches
		string tag = pRoot->getTag();
		bool bIsSwitch = tag.compare("SwitchGroup") == 0;
		bool bIsState = tag.compare("StateGroup") == 0;
		if (bIsSwitch || bIsState)
		{
			const string parent = pRoot->getAttr("Name");
			IAudioSystemItem* pGroup = m_pAudioSystemImpl->GetControlByName(parent);
			if (pGroup == nullptr)
			{
				pGroup = m_pAudioSystemImpl->CreateControl(SControlDef(parent, bIsSwitch ? eWwiseItemTypes_SwitchGroup : eWwiseItemTypes_StateGroup, false));
			}

			const XmlNodeRef pChildren = pRoot->findChild("ChildrenList");
			if (pChildren)
			{
				const int size = pChildren->getChildCount();
				for (int i = 0; i < size; ++i)
				{
					const XmlNodeRef pChild = pChildren->getChild(i);
					if (pChild)
					{
						m_pAudioSystemImpl->CreateControl(SControlDef(pChild->getAttr("Name"), bIsSwitch ? eWwiseItemTypes_Switch : eWwiseItemTypes_State, false, pGroup));
					}
				}
			}
		}

		int size = pRoot->getChildCount();
		for (int i = 0; i < size; ++i)
		{
			LoadControl(pRoot->getChild(i));
		}
	}
}

string CAudioWwiseLoader::GetLocalizationFolder() const
{
	return PathUtil::GetLocalizationFolder();
}
}
