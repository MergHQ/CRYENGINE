// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "EditorImpl.h"
#include "ImplUtils.h"

#include <IEditorImpl.h>

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/ILocalizationManager.h>

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
string const g_vcasPath = "/metadata/vca/";

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(string const& projectPath, string const& soundbanksPath, CImplItem& rootItem, ItemCache& itemCache, CEditorImpl& editorImpl)
	: m_rootItem(rootItem)
	, m_itemCache(itemCache)
	, m_projectPath(projectPath)
	, m_editorImpl(editorImpl)
{
	CImplItem* const pSoundBanksFolder = CreateItem(g_soundBanksFolderName, EImplItemType::EditorFolder, &m_rootItem);
	LoadBanks(soundbanksPath, false, *pSoundBanksFolder);

	CImplItem* const pEventsFolder = CreateItem(g_eventsFolderName, EImplItemType::EditorFolder, &m_rootItem);
	CImplItem* const pParametersFolder = CreateItem(g_parametersFolderName, EImplItemType::EditorFolder, &m_rootItem);
	CImplItem* const pSnapshotsFolder = CreateItem(g_snapshotsFolderName, EImplItemType::EditorFolder, &m_rootItem);
	CImplItem* const pReturnsFolder = CreateItem(g_returnsFolderName, EImplItemType::EditorFolder, &m_rootItem);
	CImplItem* const pVcasFolder = CreateItem(g_vcasFolderName, EImplItemType::EditorFolder, &m_rootItem);

	ParseFolder(projectPath + g_eventFoldersPath, *pEventsFolder, rootItem);          // Event folders
	ParseFolder(projectPath + g_parametersFoldersPath, *pParametersFolder, rootItem); // Parameter folders
	ParseFolder(projectPath + g_snapshotGroupsPath, *pSnapshotsFolder, rootItem);     // Snapshot groups
	ParseFolder(projectPath + g_mixerGroupsPath, *pReturnsFolder, rootItem);          // Mixer groups
	ParseFolder(projectPath + g_eventsPath, *pEventsFolder, rootItem);                // Events
	ParseFolder(projectPath + g_parametersPath, *pParametersFolder, rootItem);        // Parameters
	ParseFolder(projectPath + g_snapshotsPath, *pSnapshotsFolder, rootItem);          // Snapshots
	ParseFolder(projectPath + g_returnsPath, *pReturnsFolder, rootItem);              // Returns
	ParseFolder(projectPath + g_vcasPath, *pVcasFolder, rootItem);                    // VCAs

	RemoveEmptyMixerGroups();

	RemoveEmptyEditorFolders(pSoundBanksFolder);
	RemoveEmptyEditorFolders(pEventsFolder);
	RemoveEmptyEditorFolders(pParametersFolder);
	RemoveEmptyEditorFolders(pSnapshotsFolder);
	RemoveEmptyEditorFolders(pReturnsFolder);
	RemoveEmptyEditorFolders(pVcasFolder);
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
				CImplItem* const pSoundBank = CreateItem(filename, EImplItemType::Bank, &parent, folderPath + "/" + filename);
			}
		}

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFolder(string const& folderPath, CImplItem& editorFolder, CImplItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "*.xml", &fd);

	if (handle != -1)
	{
		do
		{
			string const filename = fd.name;

			if ((filename != ".") && (filename != "..") && !filename.empty())
			{
				ParseFile(folderPath + filename, editorFolder);
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
					else if (className == "MixerVCA")
					{
						LoadVca(pChild, parent);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::GetContainer(string const& id, EImplItemType const type, CImplItem& parent)
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
		if (type == EImplItemType::Folder)
		{
			ParseFile(m_projectPath + g_eventFoldersPath + id + ".xml", parent);
			ParseFile(m_projectPath + g_parametersFoldersPath + id + ".xml", parent);
		}
		else if (type == EImplItemType::MixerGroup)
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
CImplItem* CProjectLoader::LoadContainer(XmlNodeRef const pNode, EImplItemType const type, string const& relationshipParamName, CImplItem& parent)
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

	CImplItem* const pImplItem = CreateItem(name, EImplItemType::Folder, &parent);

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
	return LoadContainer(pNode, EImplItemType::Folder, "folder", parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadMixerGroup(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadContainer(pNode, EImplItemType::MixerGroup, "output", parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadItem(XmlNodeRef const pNode, EImplItemType const type, CImplItem& parent)
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

	if (type == EImplItemType::Snapshot)
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
	return LoadItem(pNode, EImplItemType::Event, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadSnapshot(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImplItemType::Snapshot, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadReturn(XmlNodeRef const pNode, CImplItem& parent)
{
	CImplItem* const pReturn = LoadItem(pNode, EImplItemType::Return, parent);

	if (pReturn != nullptr)
	{
		auto pParent = pReturn->GetParent();
		auto const mixerGroupType = static_cast<ItemType>(EImplItemType::MixerGroup);

		while ((pParent != nullptr) && (pParent->GetType() == mixerGroupType))
		{
			m_emptyMixerGroups.erase(std::remove(m_emptyMixerGroups.begin(), m_emptyMixerGroups.end(), pParent), m_emptyMixerGroups.end());
			pParent = pParent->GetParent();
		}
	}

	return pReturn;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadParameter(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImplItemType::Parameter, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadVca(XmlNodeRef const pNode, CImplItem& parent)
{
	return LoadItem(pNode, EImplItemType::VCA, parent);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::CreateItem(string const& name, EImplItemType const type, CImplItem* const pParent, string const& filePath /*= ""*/)
{
	CID const id = Utils::GetId(type, name, pParent, m_rootItem);
	auto pImplItem = static_cast<CImplItem*>(m_editorImpl.GetImplItem(id));

	if (pImplItem != nullptr)
	{
		if (pImplItem->IsPlaceholder())
		{
			pImplItem->SetPlaceholder(false);
			CImplItem* pParentItem = pParent;

			while (pParentItem != nullptr)
			{
				pParentItem->SetPlaceholder(false);
				pParentItem = static_cast<CImplItem*>(pParentItem->GetParent());
			}
		}
	}
	else
	{
		if (type == EImplItemType::Bank)
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type), EImplItemFlags::None, filePath);
		}
		else if (type == EImplItemType::EditorFolder)
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type), EImplItemFlags::IsContainer);
		}
		else
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type));

			if (type == EImplItemType::MixerGroup)
			{
				m_emptyMixerGroups.push_back(pImplItem);
			}
		}

		if (pParent != nullptr)
		{
			pParent->AddChild(pImplItem);
		}
		else
		{
			m_rootItem.AddChild(pImplItem);
		}

		m_itemCache[id] = pImplItem;

	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::RemoveEmptyMixerGroups()
{
	auto iter = m_emptyMixerGroups.begin();
	auto iterEnd = m_emptyMixerGroups.end();

	while (iter != iterEnd)
	{
		auto const pMixerGroup = *iter;

		if (pMixerGroup != nullptr)
		{
			auto const pParent = static_cast<CImplItem* const>(pMixerGroup->GetParent());

			if (pParent != nullptr)
			{
				pParent->RemoveChild(pMixerGroup);
			}

			size_t const numChildren = pMixerGroup->GetNumChildren();

			for (size_t i = 0; i < numChildren; ++i)
			{
				auto const pChild = static_cast<CImplItem* const>(pMixerGroup->GetChildAt(i));

				if (pChild != nullptr)
				{
					pMixerGroup->RemoveChild(pChild);
				}
			}

			auto const id = pMixerGroup->GetId();
			auto const cacheIter = m_itemCache.find(id);

			if (cacheIter != m_itemCache.end())
			{
				m_itemCache.erase(cacheIter);
			}

			delete pMixerGroup;
		}

		if (iter != (iterEnd - 1))
		{
			(*iter) = m_emptyMixerGroups.back();
		}

		m_emptyMixerGroups.pop_back();
		iter = m_emptyMixerGroups.begin();
		iterEnd = m_emptyMixerGroups.end();
	}

	m_emptyMixerGroups.clear();
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::RemoveEmptyEditorFolders(CImplItem* const pEditorFolder)
{
	if (pEditorFolder->GetNumChildren() == 0)
	{
		m_rootItem.RemoveChild(pEditorFolder);
		ItemCache::const_iterator const it(m_itemCache.find(pEditorFolder->GetId()));

		if (it != m_itemCache.end())
		{
			m_itemCache.erase(it);
		}

		delete pEditorFolder;
	}
}
} // namespace Fmod
} // namespace ACE
