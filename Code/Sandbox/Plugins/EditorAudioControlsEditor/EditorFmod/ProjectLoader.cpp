// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "EditorImpl.h"

#include <IEditorImpl.h>
#include <ImplItem.h>

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
string const g_groupsPath = "/metadata/group/";
string const g_eventsPath = "/metadata/event/";
string const g_parametersPath = "/metadata/parameterpreset/";
string const g_snapshotsPath = "/metadata/snapshot/";
string const g_returnsPath = "/metadata/return/";

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(string const& projectPath, string const& soundbanksPath, CImplItem& root)
	: m_root(root)
	, m_projectPath(projectPath)
{
	LoadBanks(soundbanksPath);

	ParseFolder(projectPath + g_eventFoldersPath);      // event folders
	ParseFolder(projectPath + g_parametersFoldersPath); // event folders
	ParseFolder(projectPath + g_groupsPath);            // groups
	ParseFolder(projectPath + g_eventsPath);            // events
	ParseFolder(projectPath + g_parametersPath);        // parameters
	ParseFolder(projectPath + g_snapshotsPath);         // snapshots
	ParseFolder(projectPath + g_returnsPath);           // returns
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadBanks(string const& folderPath)
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
				CImplItem* const pSoundBank = CreateItem(EImpltemType::Bank, nullptr, filename);
				pSoundBank->SetFilePath(folderPath + CRY_NATIVE_PATH_SEPSTR + filename);
			}
		}

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFolder(string const& folderPath)
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
				ParseFile(folderPath + filename);
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseFile(string const& filepath)
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
						pImplItem = LoadFolder(pChild);
					}
					else if (className == "Event")
					{
						pImplItem = LoadEvent(pChild);
					}
					else if (className == "Snapshot")
					{
						pImplItem = LoadSnapshot(pChild);
					}
					else if (className == "ParameterPreset")
					{
						pImplItem = LoadParameter(pChild);
					}
					else if (className == "MixerReturn")
					{
						LoadReturn(pChild);
					}
					else if (className == "MixerGroup")
					{
						LoadGroup(pChild);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::GetContainer(string const& id, EImpltemType const type)
{
	CImplItem* pImplItem = nullptr;
	auto folder = m_containerIdMap.find(id);

	if (folder != m_containerIdMap.end())
	{
		pImplItem =(*folder).second;
	}
	else
	{
		// If folder not found parse the file corresponding to it and try looking for it again
		if (type == EImpltemType::Folder)
		{
			ParseFile(m_projectPath + g_eventFoldersPath + id + ".xml");
		}
		else if (type == EImpltemType::Group)
		{
			ParseFile(m_projectPath + g_groupsPath + id + ".xml");
		}

		folder = m_containerIdMap.find(id);

		if (folder != m_containerIdMap.end())
		{
			pImplItem =(*folder).second;
		}
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadContainer(XmlNodeRef const pNode, EImpltemType const type, string const& relationshipParamName)
{
	CImplItem* pImplItem = nullptr;
	string containerName = "";
	CImplItem* pParent = nullptr;
	int const size = pNode->getChildCount();

	for (int i = 0; i < size; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);
		string const name = pChild->getAttr("name");

		if (name == "name")
		{
			// Get the container name
			XmlNodeRef const pContainerNameNode = pChild->getChild(0);

			if (pContainerNameNode != nullptr)
			{
				containerName = pContainerNameNode->getContent();
			}
		}
		else if (name == relationshipParamName)
		{
			// Get the container parent
			XmlNodeRef const pParentContainerNode = pChild->getChild(0);

			if (pParentContainerNode != nullptr)
			{
				string const parentContainerId = pParentContainerNode->getContent();
				pParent = GetContainer(parentContainerId, type);
			}
		}
	}

	CImplItem* const pContainer = CreateItem(type, pParent, containerName);
	m_containerIdMap[pNode->getAttr("id")] = pContainer;
	pImplItem = pContainer;

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadFolder(XmlNodeRef const pNode)
{
	return LoadContainer(pNode, EImpltemType::Folder, "folder");
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadGroup(XmlNodeRef const pNode)
{
	return LoadContainer(pNode, EImpltemType::Group, "output");
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadItem(XmlNodeRef const pNode, EImpltemType const type)
{
	CImplItem* pImplItem = nullptr;
	string name = "";
	CImplItem* pParent = nullptr;
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
						name = pValue->getContent();
					}
				}
			}
			else if (tag == "relationship")
			{
				string const name = pChild->getAttr("name");

				if (name == "folder" || name == "output")
				{
					XmlNodeRef const pValue = pChild->getChild(0);

					if (pValue != nullptr)
					{
						string const parentContainerId = pValue->getContent();
						auto const folder = m_containerIdMap.find(parentContainerId);

						if (folder != m_containerIdMap.end())
						{
							pParent = (*folder).second;
						}
					}
				}
			}
		}
	}

	pImplItem = CreateItem(type, pParent, name);

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadEvent(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Event);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadSnapshot(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Snapshot);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadReturn(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Return);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::LoadParameter(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Parameter);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::CreateItem(EImpltemType const type, CImplItem* const pParent, string const& name)
{
	CID const id = GetId(type, name, pParent);
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
		else if (type == EImpltemType::Group)
		{
			pImplItem = new CImplGroup(name, id);
		}
		else
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type));
		}
		if (pParent != nullptr)
		{
			pParent->AddChild(pImplItem);
			pImplItem->SetParent(pParent);
		}
		else
		{
			m_root.AddChild(pImplItem);
			pImplItem->SetParent(&m_root);
		}

		m_controlsCache[id] = pImplItem;

	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CID CProjectLoader::GetId(EImpltemType const type, string const& name, CImplItem* const pParent) const
{
	string const fullname = GetTypeName(type) + GetPathName(pParent) + CRY_NATIVE_PATH_SEPSTR + name;
	return CryAudio::StringToId(fullname);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CProjectLoader::GetControl(CID const id) const
{
	CImplItem* pImplItem = nullptr;

	for (auto const controlPair : m_controlsCache)
	{
		CImplItem* const pImplControl = controlPair.second;

		if (pImplControl->GetId() == id)
		{
			pImplItem = pImplControl;
			break;
		}
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
string CProjectLoader::GetPathName(CImplItem const* const pImplItem) const
{
	string pathName = "";

	if (pImplItem != nullptr)
	{
		string fullname = pImplItem->GetName();
		CImplItem const* pParent = pImplItem->GetParent();

		while ((pParent != nullptr) && (pParent != &m_root))
		{
			// The id needs to represent the full path, as we can have items with the same name in different folders
			fullname = pParent->GetName() + "/" + fullname;
			pParent = pParent->GetParent();
		}

		pathName = fullname;
	}

	return pathName;
}

//////////////////////////////////////////////////////////////////////////
string CProjectLoader::GetTypeName(EImpltemType const type) const
{
	string typeName = "";

	switch (type)
	{
	case EImpltemType::Folder:
		typeName = "folder:";
		break;
	case EImpltemType::Event:
		typeName = "event:";
		break;
	case EImpltemType::Parameter:
		typeName = "parameter:";
		break;
	case EImpltemType::Snapshot:
		typeName = "snapshot:";
		break;
	case EImpltemType::Bank:
		typeName = "bank:";
		break;
	case EImpltemType::Return:
		typeName = "return:";
		break;
	case EImpltemType::Group:
		typeName = "group:";
		break;
	default:
		typeName = "";
		break;
	}

	return typeName;
}
} // namespace Fmod
} // namespace ACE
