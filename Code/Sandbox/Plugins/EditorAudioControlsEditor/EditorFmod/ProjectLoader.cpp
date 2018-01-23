// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "EditorImpl.h"
#include "ImplUtils.h"

#include <IEditorImpl.h>

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/ILocalizationManager.h>
#include <CryString/CryPath.h>

namespace ACE
{
namespace Fmod
{
// Paths
string const g_eventFoldersPath = "/metadata/eventfolder/";
string const g_parametersFoldersPath = "/metadata/parameterpresetfolder/";
string const g_snapshotGroupsPath = "/metadata/snapshotgroup/";
string const g_mixerGroupsPath = "/metadata/group/";
string const g_eventsPath = "/metadata/event/";
string const g_parametersPath = "/metadata/parameterpreset/";
string const g_snapshotsPath = "/metadata/snapshot/";
string const g_returnsPath = "/metadata/return/";

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(string const& projectPath, string const& soundbanksPath, CImplItem& root)
	: m_root(root)
	, m_projectPath(projectPath)
{
	CImplItem* const pSoundBanks = CreateItem(g_soundBanksFolderName, EImpltemType::EditorFolder, &m_root);
	LoadBanks(soundbanksPath, false, *pSoundBanks);

	ParseFolder(projectPath + g_eventFoldersPath, g_eventsFolderName, root);          // event folders
	ParseFolder(projectPath + g_parametersFoldersPath, g_parametersFolderName, root); // parameter folders
	ParseFolder(projectPath + g_snapshotGroupsPath, g_snapshotsFolderName, root);     // snapshot groups
	ParseFolder(projectPath + g_mixerGroupsPath, g_returnsFolderName, root);          // mixer groups
	ParseFolder(projectPath + g_eventsPath, g_eventsFolderName, root);                // events
	ParseFolder(projectPath + g_parametersPath, g_parametersFolderName, root);        // parameters
	ParseFolder(projectPath + g_snapshotsPath, g_snapshotsFolderName, root);          // snapshots
	ParseFolder(projectPath + g_returnsPath, g_returnsFolderName, root);              // returns
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadBanks(string const& folderPath, bool const isLocalized, CImplItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*.bank", &fd);

	if (handle != -1)
	{
		// We have to exclude the Master Bank, for this we look
		// for the file that ends with "strings.bank" as it is guaranteed
		// to have the same name as the Master Bank and there should be unique
		std::vector<string> banks;
		string masterBankName = "";

		do
		{
			string const filename = fd.name;

			if ((filename != ".") && (filename != "..") && !filename.empty())
			{
				int const pos = filename.rfind(".strings.bank");

				if (pos != string::npos)
				{
					masterBankName = filename.substr(0, pos);
				}
				else
				{
					banks.emplace_back(filename);
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		for (string const& filename : banks)
		{
			if (filename.compare(0, masterBankName.length(), masterBankName) != 0)
			{
				CImplItem* const pSoundBank = CreateItem(filename, EImpltemType::Bank, &parent);
				pSoundBank->SetFilePath(folderPath + CRY_NATIVE_PATH_SEPSTR + filename);
			}
		}

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFolder(string const& folderPath, string const& folderName, CImplItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "*.xml", &fd);

	if (handle != -1)
	{
		CImplItem* const pEditorFolder = CreateItem(folderName, EImpltemType::EditorFolder, &parent);

		do
		{
			string const filename = fd.name;

			if ((filename != ".") && (filename != "..") && !filename.empty())
			{
				ParseFile(folderPath + filename, *pEditorFolder);
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFile(string const& filepath, CImplItem& parent)
{
	if (GetISystem()->GetIPak()->IsFileExist(filepath))
	{
		XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(filepath);

		if (pRoot != nullptr)
		{
			CImplItem* pImplItem = nullptr;
			float maxDistance = 0.0f;
			int const size = pRoot->getChildCount();

			for (int i = 0; i < size; ++i)
			{
				XmlNodeRef const pChild = pRoot->getChild(i);

				if (pChild != nullptr)
				{
					string const className = pChild->getAttr("class");

					if ((className == "EventFolder") || (className == "ParameterPresetFolder"))
					{
						pImplItem = LoadFolder(pChild, parent);
					}
					else if (className == "SnapshotGroup")
					{
						LoadSnapshotGroup(pChild, parent);
					}
					else if (className == "Event")
					{
						pImplItem = LoadEvent(pChild, parent);
					}
					else if (className == "Snapshot")
					{
						pImplItem = LoadSnapshot(pChild, parent);
					}
					else if (className == "ParameterPreset")
					{
						pImplItem = LoadParameter(pChild, parent);
					}
					else if (className == "MixerReturn")
					{
						LoadReturn(pChild, parent);
					}
					else if (className == "MixerGroup")
					{
						LoadMixerGroup(pChild, parent);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::GetContainer(string const& id, EImpltemType const type, CImplItem& parent)
{
	CImplItem* pImplItem = &parent;
	auto folder = m_containerIds.find(id);

	if (folder != m_containerIds.end())
	{
		pImplItem = (*folder).second;
	}
	else
	{
		// If folder not found parse the file corresponding to it and try looking for it again
		if (type == EImpltemType::Folder)
		{
			ParseFile(m_projectPath + g_eventFoldersPath + id + ".xml", parent);
			ParseFile(m_projectPath + g_parametersFoldersPath + id + ".xml", parent);
		}
		else if (type == EImpltemType::MixerGroup)
		{
			ParseFile(m_projectPath + g_mixerGroupsPath + id + ".xml", parent);
		}

		folder = m_containerIds.find(id);

		if (folder != m_containerIds.end())
		{
			pImplItem = (*folder).second;
		}
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadContainer(XmlNodeRef const pNode, EImpltemType const type, string const& relationshipParamName, CImplItem& parent)
{
	CImplItem* pImplItem = nullptr;
	string name = "";
	CImplItem* pParent = &parent;
	int const size = pNode->getChildCount();

	for (int i = 0; i < size; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);
		string const attribName = pChild->getAttr("name");

		if (attribName == "name")
		{
			// Get the container name
			XmlNodeRef const pNameNode = pChild->getChild(0);

			if (pNameNode != nullptr)
			{
				name = pNameNode->getContent();
			}
		}
		else if (attribName == relationshipParamName)
		{
			// Get the container parent
			XmlNodeRef const pParentContainerNode = pChild->getChild(0);

			if (pParentContainerNode != nullptr)
			{
				string const parentContainerId = pParentContainerNode->getContent();
				pParent = GetContainer(parentContainerId, type, parent);
			}
		}
	}

	CImplItem* const pContainer = CreateItem(name, type, pParent);
	m_containerIds[pNode->getAttr("id")] = pContainer;
	pImplItem = pContainer;

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadSnapshotGroup(XmlNodeRef const pNode, CImplItem& parent)
{
	string name = "";
	std::vector<string> snapshotsItems;
	int const size = pNode->getChildCount();

	for (int i = 0; i < size; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);
		string const attribName = pChild->getAttr("name");

		if (attribName == "name")
		{
			XmlNodeRef const pNameNode = pChild->getChild(0);

			if (pNameNode != nullptr)
			{
				name = pNameNode->getContent();
			}
		}
		else if (attribName == "items")
		{
			int const itemCount = pChild->getChildCount();

			for (int j = 0; j < itemCount; ++j)
			{
				XmlNodeRef const itemNode = pChild->getChild(j);

				if (itemNode != nullptr)
				{
					snapshotsItems.emplace_back(itemNode->getContent());
				}
			}
		}
	}

	CImplItem* const pImplItem = CreateItem(name, EImpltemType::Folder, &parent);

	if (!snapshotsItems.empty())
	{
		for (auto const& snapshotId : snapshotsItems)
		{
			m_snapshotGroupItems[snapshotId] = pImplItem;
		}
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadFolder(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadContainer(pNode, EImpltemType::Folder, "folder", parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadMixerGroup(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadContainer(pNode, EImpltemType::MixerGroup, "output", parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadItem(XmlNodeRef const pNode, EImpltemType const type, CImplItem& parent)
{
	CImplItem* pImplItem = nullptr;
	string itemName = "";
	CImplItem* pParent = &parent;
	int const size = pNode->getChildCount();

	for (int i = 0; i < size; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			string const tag = pChild->getTag();

			if (tag == "property")
			{
				string const paramName = pChild->getAttr("name");

				if (paramName == "name")
				{
					XmlNodeRef const pValue = pChild->getChild(0);

					if (pValue != nullptr)
					{
						itemName = pValue->getContent();
					}
				}
			}
			else if (tag == "relationship")
			{
				string const relationshipName = pChild->getAttr("name");

				if (relationshipName == "folder" || relationshipName == "output")
				{
					XmlNodeRef const pValue = pChild->getChild(0);

					if (pValue != nullptr)
					{
						string const parentContainerId = pValue->getContent();
						auto const folder = m_containerIds.find(parentContainerId);

						if (folder != m_containerIds.end())
						{
							pParent = (*folder).second;
						}
					}
				}
			}
		}
	}

	if (type == EImpltemType::Snapshot)
	{
		string const id = pNode->getAttr("id");

		auto const snapshotGroupPair = m_snapshotGroupItems.find(id);

		if (snapshotGroupPair != m_snapshotGroupItems.end())
		{
			pParent = (*snapshotGroupPair).second;
		}
	}

	pImplItem = CreateItem(itemName, type, pParent);

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadEvent(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImpltemType::Event, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadSnapshot(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImpltemType::Snapshot, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadReturn(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImpltemType::Return, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadParameter(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImpltemType::Parameter, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::CreateItem(string const& name, EImpltemType const type, CImplItem* const pParent)
{
	CID const id = Utils::GetId(type, name, pParent, m_root);
	CImplItem* pImplItem = GetControl(id);

	if (pImplItem != nullptr)
	{
		if (pImplItem->IsPlaceholder())
		{
			pImplItem->SetPlaceholder(false);
			CImplItem* pParentItem = pParent;

			while (pParentItem != nullptr)
			{
				pParentItem->SetPlaceholder(false);
				pParentItem = pParentItem->GetParent();
			}
		}
	}
	else
	{
		if (type == EImpltemType::Folder)
		{
			pImplItem = new CImplFolder(name, id);
		}
		else if (type == EImpltemType::MixerGroup)
		{
			pImplItem = new CImplMixerGroup(name, id);
		}
		else
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type));

			if (type == EImpltemType::EditorFolder)
			{
				pImplItem->SetContainer(true);
			}
		}

		if (pParent != nullptr)
		{
			pParent->AddChild(pImplItem);
		}
		else
		{
			m_root.AddChild(pImplItem);
		}

		m_controlsCache[id] = pImplItem;

	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::GetControl(CID const id) const
{
	CImplItem* pImplItem = nullptr;

	if (id >= 0)
	{
		pImplItem = stl::find_in_map(m_controlsCache, id, nullptr);
	}

	return pImplItem;
}
} // namespace Fmod
} // namespace ACE
