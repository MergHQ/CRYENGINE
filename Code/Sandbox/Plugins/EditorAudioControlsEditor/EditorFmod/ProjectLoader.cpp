// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "Impl.h"
#include "Utils.h"

#include <CryAudioImplFmod/GlobalData.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>

namespace ACE
{
namespace Impl
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
string const g_bankPath = "/metadata/bank/";

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(string const& projectPath, string const& banksPath, string const& localizedBanksPath, CItem& rootItem, ItemCache& itemCache, CImpl const& impl)
	: m_rootItem(rootItem)
	, m_itemCache(itemCache)
	, m_projectPath(projectPath)
	, m_impl(impl)
{
	CItem* const pSoundBanksFolder = CreateItem(s_soundBanksFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
	LoadBanks(banksPath, false, *pSoundBanksFolder);
	LoadBanks(localizedBanksPath, true, *pSoundBanksFolder);

	CItem* const pEventsFolder = CreateItem(s_eventsFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
	CItem* const pParametersFolder = CreateItem(s_parametersFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
	CItem* const pSnapshotsFolder = CreateItem(s_snapshotsFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
	CItem* const pReturnsFolder = CreateItem(s_returnsFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
	CItem* const pVcasFolder = CreateItem(s_vcasFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);

	ParseFolder(projectPath + g_eventFoldersPath, *pEventsFolder, rootItem);          // Event folders
	ParseFolder(projectPath + g_parametersFoldersPath, *pParametersFolder, rootItem); // Parameter folders
	ParseFolder(projectPath + g_snapshotGroupsPath, *pSnapshotsFolder, rootItem);     // Snapshot groups
	ParseFolder(projectPath + g_mixerGroupsPath, *pReturnsFolder, rootItem);          // Mixer groups
	ParseFolder(projectPath + g_eventsPath, *pEventsFolder, rootItem);                // Events
	ParseFolder(projectPath + g_parametersPath, *pParametersFolder, rootItem);        // Parameters
	ParseFolder(projectPath + g_snapshotsPath, *pSnapshotsFolder, rootItem);          // Snapshots
	ParseFolder(projectPath + g_returnsPath, *pReturnsFolder, rootItem);              // Returns
	ParseFolder(projectPath + g_vcasPath, *pVcasFolder, rootItem);                    // VCAs
	ParseFolder(projectPath + g_bankPath, rootItem, rootItem);                        // Audio tables of banks

	if (!m_audioTableDirectories.empty())
	{
		CItem* const pKeysFolder = CreateItem(s_keysFolderName, EItemType::EditorFolder, &m_rootItem, EItemFlags::IsContainer);
		LoadKeys(*pKeysFolder);
		RemoveEmptyEditorFolders(pKeysFolder);
	}

	RemoveEmptyMixerGroups();

	RemoveEmptyEditorFolders(pSoundBanksFolder);
	RemoveEmptyEditorFolders(pEventsFolder);
	RemoveEmptyEditorFolders(pParametersFolder);
	RemoveEmptyEditorFolders(pSnapshotsFolder);
	RemoveEmptyEditorFolders(pReturnsFolder);
	RemoveEmptyEditorFolders(pVcasFolder);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadBanks(string const& folderPath, bool const isLocalized, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*.bank", &fd);

	if (handle != -1)
	{
		EItemFlags const flags = isLocalized ? EItemFlags::IsLocalized : EItemFlags::None;

		// We have to exclude the Master Bank, for this we look
		// for the file that ends with "strings.bank" as it is guaranteed
		// to have the same name as the Master Bank and there should be unique
		AssetNames banks;
		string masterBankName = "";

		do
		{
			string const filename = fd.name;

			if ((filename != ".") && (filename != "..") && !filename.empty())
			{
				if (isLocalized)
				{
					banks.emplace_back(filename);
				}
				else
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
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		for (string const& filename : banks)
		{
			if (isLocalized || (filename.compareNoCase(0, masterBankName.length(), masterBankName) != 0))
			{
				string const filePath = folderPath + "/" + filename;
				EPakStatus const pakStatus = pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_OnDisk) ? EPakStatus::OnDisk : EPakStatus::None;

				CreateItem(filename, EItemType::Bank, &parent, flags, pakStatus, filePath);
			}
		}

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFolder(string const& folderPath, CItem& editorFolder, CItem& parent)
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
				string const filePath = folderPath + filename;
				EPakStatus const pakStatus = pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_OnDisk) ? EPakStatus::OnDisk : EPakStatus::None;

				ParseFile(filePath, editorFolder, pakStatus);
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFile(string const& filepath, CItem& parent, EPakStatus const pakStatus)
{
	if (GetISystem()->GetIPak()->IsFileExist(filepath))
	{
		XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(filepath);

		if (pRoot != nullptr)
		{
			CItem* pItem = nullptr;
			int const numChildren = pRoot->getChildCount();

			for (int i = 0; i < numChildren; ++i)
			{
				XmlNodeRef const pChild = pRoot->getChild(i);

				if (pChild != nullptr)
				{
					char const* const szClassName = pChild->getAttr("class");

					if ((_stricmp(szClassName, "EventFolder") == 0) || (_stricmp(szClassName, "ParameterPresetFolder") == 0))
					{
						LoadFolder(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "SnapshotGroup") == 0)
					{
						LoadSnapshotGroup(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "Event") == 0)
					{
						pItem = LoadEvent(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "Snapshot") == 0)
					{
						LoadSnapshot(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "ParameterPreset") == 0)
					{
						LoadParameter(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "MixerReturn") == 0)
					{
						LoadReturn(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "MixerGroup") == 0)
					{
						LoadMixerGroup(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "MixerVCA") == 0)
					{
						LoadVca(pChild, parent, pakStatus);
					}
					else if (_stricmp(szClassName, "AudioTable") == 0)
					{
						LoadAudioTable(pChild);
					}
					else if (_stricmp(szClassName, "ProgrammerSound") == 0)
					{
						if ((pItem != nullptr) && (pItem->GetType() == EItemType::Event))
						{
							string const pathName = Utils::GetPathName(pItem, m_rootItem);
							CImpl::s_programmerSoundEvents.emplace_back(pathName);
						}
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseKeysFile(string const& filePath, CItem& parent)
{
	CCryFile file;

	if (file.Open(filePath, "r"))
	{
		size_t const length = file.GetLength();
		string allText;

		if (length > 0)
		{
			std::vector<char> buffer;
			buffer.resize(length, '\n');
			file.ReadRaw(&buffer[0], length);
			allText.assign(&buffer[0], length);

			int linePos = 0;
			string line;

			while (!(line = allText.Tokenize("\r\n", linePos)).empty())
			{
				int keyPos = 0;
				string const key = line.Tokenize(",", keyPos).Trim();
				CreateItem(key, EItemType::Key, &parent, EItemFlags::None);
			}
		}

		file.Close();
	}
}

//////////////////////////////////////////////////////////////////////////
CItem* CProjectLoader::GetContainer(string const& id, EItemType const type, CItem& parent, EPakStatus const pakStatus)
{
	CItem* pItem = &parent;
	auto folder = m_containerIds.find(id);

	if (folder != m_containerIds.end())
	{
		pItem = (*folder).second;
	}
	else
	{
		// If folder not found parse the file corresponding to it and try looking for it again
		if (type == EItemType::Folder)
		{
			ParseFile(m_projectPath + g_eventFoldersPath + id + ".xml", parent, pakStatus);
			ParseFile(m_projectPath + g_parametersFoldersPath + id + ".xml", parent, pakStatus);
		}
		else if (type == EItemType::MixerGroup)
		{
			ParseFile(m_projectPath + g_mixerGroupsPath + id + ".xml", parent, pakStatus);
		}

		folder = m_containerIds.find(id);

		if (folder != m_containerIds.end())
		{
			pItem = (*folder).second;
		}
	}

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadContainer(XmlNodeRef const pNode, EItemType const type, string const& relationshipParamName, CItem& parent, EPakStatus const pakStatus)
{
	string name = "";
	CItem* pParent = &parent;
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);
		string const attribName = pChild->getAttr("name");

		if (attribName.compareNoCase("name") == 0)
		{
			// Get the container name
			XmlNodeRef const pNameNode = pChild->getChild(0);

			if (pNameNode != nullptr)
			{
				name = pNameNode->getContent();
			}
		}
		else if (attribName.compareNoCase(relationshipParamName) == 0)
		{
			// Get the container parent
			XmlNodeRef const pParentContainerNode = pChild->getChild(0);

			if (pParentContainerNode != nullptr)
			{
				string const parentContainerId = pParentContainerNode->getContent();
				pParent = GetContainer(parentContainerId, type, parent, pakStatus);
			}
		}
	}

	CItem* const pContainer = CreateItem(name, type, pParent, EItemFlags::IsContainer, pakStatus);
	m_containerIds[pNode->getAttr("id")] = pContainer;
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadSnapshotGroup(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	string name = "";
	AssetNames snapshotsItems;
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);
		char const* const szAttribName = pChild->getAttr("name");

		if (_stricmp(szAttribName, "name") == 0)
		{
			XmlNodeRef const pNameNode = pChild->getChild(0);

			if (pNameNode != nullptr)
			{
				name = pNameNode->getContent();
			}
		}
		else if (_stricmp(szAttribName, "items") == 0)
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

	CItem* const pItem = CreateItem(name, EItemType::Folder, &parent, EItemFlags::IsContainer, pakStatus);

	if (!snapshotsItems.empty())
	{
		for (auto const& snapshotId : snapshotsItems)
		{
			m_snapshotGroupItems[snapshotId] = pItem;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadFolder(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	LoadContainer(pNode, EItemType::Folder, "folder", parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadMixerGroup(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	LoadContainer(pNode, EItemType::MixerGroup, "output", parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
CItem* CProjectLoader::LoadItem(XmlNodeRef const pNode, EItemType const type, CItem& parent, EPakStatus const pakStatus)
{
	CItem* pItem = nullptr;
	string itemName = "";
	CItem* pParent = &parent;
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			char const* const szTag = pChild->getTag();

			if (_stricmp(szTag, "property") == 0)
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
			else if (_stricmp(szTag, "relationship") == 0)
			{
				char const* const relationshipName = pChild->getAttr("name");

				if ((_stricmp(relationshipName, "folder") == 0) || (_stricmp(relationshipName, "output") == 0))
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

	if (type == EItemType::Snapshot)
	{
		string const id = pNode->getAttr("id");

		auto const snapshotGroupPair = m_snapshotGroupItems.find(id);

		if (snapshotGroupPair != m_snapshotGroupItems.end())
		{
			pParent = (*snapshotGroupPair).second;
		}
	}

	pItem = CreateItem(itemName, type, pParent, EItemFlags::None, pakStatus);

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
CItem* CProjectLoader::LoadEvent(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	return LoadItem(pNode, EItemType::Event, parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadSnapshot(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	LoadItem(pNode, EItemType::Snapshot, parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadReturn(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	CItem* const pReturn = LoadItem(pNode, EItemType::Return, parent, pakStatus);

	if (pReturn != nullptr)
	{
		auto pParent = static_cast<CItem*>(pReturn->GetParent());

		while ((pParent != nullptr) && (pParent->GetType() == EItemType::MixerGroup))
		{
			m_emptyMixerGroups.erase(std::remove(m_emptyMixerGroups.begin(), m_emptyMixerGroups.end(), pParent), m_emptyMixerGroups.end());
			pParent = static_cast<CItem*>(pParent->GetParent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadParameter(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	LoadItem(pNode, EItemType::Parameter, parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadVca(XmlNodeRef const pNode, CItem& parent, EPakStatus const pakStatus)
{
	LoadItem(pNode, EItemType::VCA, parent, pakStatus);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadAudioTable(XmlNodeRef const pNode)
{
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			char const* const szTag = pChild->getTag();

			if (_stricmp(szTag, "property") == 0)
			{
				string const paramName = pChild->getAttr("name");

				if (paramName == "sourceDirectory")
				{
					XmlNodeRef const pValue = pChild->getChild(0);

					if (pValue != nullptr)
					{
						m_audioTableDirectories.emplace(m_projectPath + "/" + pValue->getContent());
						break;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadKeys(CItem& parent)
{
	for (auto const& tableDir : m_audioTableDirectories)
	{
		string const keysFilePath = tableDir + "/keys.txt";

		_finddata_t fd;
		ICryPak* const pCryPak = gEnv->pCryPak;

		if (pCryPak->IsFileExist(keysFilePath.c_str()))
		{
			ParseKeysFile(keysFilePath, parent);
		}
		else
		{
			intptr_t const handle = pCryPak->FindFirst(tableDir + "/*.*", &fd);

			if (handle != -1)
			{
				do
				{
					string fileName = fd.name;

					if ((fileName != ".") && (fileName != "..") && !fileName.empty())
					{
						string::size_type const posExtension = fileName.rfind('.');

						if (posExtension != string::npos)
						{
							string const fileExtension = fileName.data() + posExtension;

							// TODO: Add all supported file formats.
							if ((_stricmp(fileExtension, ".mp3") == 0) ||
							    (_stricmp(fileExtension, ".ogg") == 0) ||
							    (_stricmp(fileExtension, ".wav") == 0) ||
							    (_stricmp(fileExtension, ".mp2") == 0) ||
							    (_stricmp(fileExtension, ".flac") == 0) ||
							    (_stricmp(fileExtension, ".aiff") == 0))
							{
								PathUtil::RemoveExtension(fileName);
								CreateItem(fileName, EItemType::Key, &parent, EItemFlags::None);
							}
						}
					}
				}
				while (pCryPak->FindNext(handle, &fd) >= 0);

				pCryPak->FindClose(handle);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CItem* CProjectLoader::CreateItem(
	string const& name,
	EItemType const type,
	CItem* const pParent,
	EItemFlags const flags,
	EPakStatus const pakStatus /*= EPakStatus::None*/,
	string const& filePath /*= ""*/)
{
	ControlId const id = Utils::GetId(type, name, pParent, m_rootItem);
	auto pItem = static_cast<CItem*>(m_impl.GetItem(id));

	if (pItem == nullptr)
	{
		pItem = new CItem(name, id, type, flags, pakStatus, filePath);

		if (type == EItemType::MixerGroup)
		{
			m_emptyMixerGroups.push_back(pItem);
		}

		if (pParent != nullptr)
		{
			pParent->AddChild(pItem);
		}
		else
		{
			m_rootItem.AddChild(pItem);
		}

		m_itemCache[id] = pItem;
	}

	return pItem;
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
			auto const pParent = static_cast<CItem* const>(pMixerGroup->GetParent());

			if (pParent != nullptr)
			{
				pParent->RemoveChild(pMixerGroup);
			}

			size_t const numChildren = pMixerGroup->GetNumChildren();

			for (size_t i = 0; i < numChildren; ++i)
			{
				auto const pChild = static_cast<CItem* const>(pMixerGroup->GetChildAt(i));

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
void CProjectLoader::RemoveEmptyEditorFolders(CItem* const pEditorFolder)
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
} // namespace Impl
} // namespace ACE
