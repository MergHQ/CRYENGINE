// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "Impl.h"
#include "Utils.h"

#include <CryAudioImplAdx2/GlobalData.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
static string const s_nameAttrib = "OrcaName";
static string const s_typeAttrib = "OrcaType";

//////////////////////////////////////////////////////////////////////////
XmlNodeRef FindNodeByAttributeValue(XmlNodeRef const pRoot, string const attributeValue)
{
	XmlNodeRef pNode = nullptr;

	if (pRoot != nullptr)
	{
		int const numChildren = pRoot->getChildCount();

		for (int i = 0; i < numChildren; ++i)
		{
			XmlNodeRef const pChild = pRoot->getChild(i);

			if (pChild != nullptr)
			{
				string const nameValue = pChild->getAttr(s_nameAttrib);

				if (nameValue.compareNoCase(attributeValue) == 0)
				{
					pNode = pChild;
					break;
				}
				else if (pChild->getChildCount() > 0)
				{
					pNode = FindNodeByAttributeValue(pChild, attributeValue);

					if (pNode != nullptr)
					{
						break;
					}
				}
			}
		}
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(
	string const& projectPath,
	string const& assetsPath,
	string const& localizedAssetsPath,
	CItem& rootItem,
	ItemCache& itemCache,
	CImpl const& impl)
	: m_rootItem(rootItem)
	, m_itemCache(itemCache)
	, m_impl(impl)
	, m_pBusesFolder(nullptr)
	, m_pSnapShotsFolder(nullptr)
{
	LoadGlobalSettings(projectPath, rootItem);

	CItem* const pWorkUnitsFolder = CreateItem(s_workUnitsFolderName, EItemType::FolderCueSheet, &m_rootItem, EItemFlags::IsContainer);
	LoadWorkUnits(projectPath + "/WorkUnits/", *pWorkUnitsFolder);

	CItem* const pBinariesFolder = CreateItem(s_binariesFolderName, EItemType::FolderCueSheet, &m_rootItem, EItemFlags::IsContainer);
	LoadBinaries(assetsPath, false, *pBinariesFolder);
	LoadBinaries(localizedAssetsPath, true, *pBinariesFolder);

	RemoveEmptyFolder(pWorkUnitsFolder);
	RemoveEmptyFolder(pBinariesFolder);
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
		if (type == EItemType::Cue)
		{
			pItem = new CItem(name, id, type, flags, pakStatus, filePath, Utils::FindCueSheetName(pParent, m_rootItem));
		}
		else
		{
			pItem = new CItem(name, id, type, flags, pakStatus, filePath);
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
void CProjectLoader::LoadGlobalSettings(string const& folderPath, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*.atmcglobal", &fd);

	if (handle != -1)
	{
		do
		{
			string const fileName = fd.name;
			XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(folderPath + "/" + fileName);

			if (pRoot != nullptr)
			{
				XmlNodeRef const pGlobalSettingsNode = FindNodeByAttributeValue(pRoot, "GlobalSettings");

				if (pGlobalSettingsNode != nullptr)
				{
					CItem* const pGlobalSettingsFolder = CreateItem(s_globalSettingsFolderName, EItemType::FolderGlobal, &m_rootItem, EItemFlags::IsContainer);
					int const numChildren = pGlobalSettingsNode->getChildCount();

					for (int i = 0; i < numChildren; ++i)
					{
						XmlNodeRef const pChild = pGlobalSettingsNode->getChild(i);

						if ((pChild != nullptr) && (pChild->getChildCount() > 0))
						{
							char const* const szNameValue = pChild->getAttr(s_nameAttrib);

							if (_stricmp(szNameValue, "AISAC-Controls") == 0)
							{
								CItem* const pFolder = CreateItem(s_aisacControlsFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								ParseGlobalSettingsFile(pChild, *pFolder, EItemType::AisacControl);
							}
							else if (_stricmp(szNameValue, "GameVariables") == 0)
							{
								CItem* const pFolder = CreateItem(s_gameVariablesFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								ParseGlobalSettingsFile(pChild, *pFolder, EItemType::GameVariable);
							}
							else if (_stricmp(szNameValue, "SelectorFolder") == 0)
							{
								CItem* const pFolder = CreateItem(s_selectorsFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								ParseGlobalSettingsFile(pChild, *pFolder, EItemType::Selector);
							}
							else if (_stricmp(szNameValue, "Categories") == 0)
							{
								CItem* const pFolder = CreateItem(s_categoriesFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								ParseGlobalSettingsFile(pChild, *pFolder, EItemType::CategoryGroup);
							}
							else if (_stricmp(szNameValue, "DspBusSettings") == 0)
							{
								m_pBusesFolder = CreateItem(s_dspBusesFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								m_pSnapShotsFolder = CreateItem(s_snapshotsFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);

								CItem* const pFolder = CreateItem(s_dspBusSettingsFolderName, EItemType::FolderGlobal, pGlobalSettingsFolder, EItemFlags::IsContainer);
								ParseBusSettings(pChild, *pFolder);
							}
						}
					}
				}
			}

			break;
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseGlobalSettingsFile(XmlNodeRef const pNode, CItem& parent, EItemType const type)
{
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			char const* const szNameValue = pChild->getAttr(s_nameAttrib);
			CItem* const pItem = CreateItem(szNameValue, type, &parent, EItemFlags::None);

			if (type == EItemType::Selector)
			{
				pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsContainer);
				ParseGlobalSettingsFile(pChild, *pItem, EItemType::SelectorLabel);
			}
			else if (type == EItemType::CategoryGroup)
			{
				pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsContainer);
				ParseGlobalSettingsFile(pChild, *pItem, EItemType::Category);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseBusSettings(XmlNodeRef const pNode, CItem& parent)
{
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			char const* const szNameValue = pChild->getAttr(s_nameAttrib);
			CItem* const pItem = CreateItem(szNameValue, EItemType::DspBusSetting, &parent, EItemFlags::None);
			ParseBusesAndSnapshots(pChild);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseBusesAndSnapshots(XmlNodeRef const pNode)
{
	int const numChildren = pNode->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pNode->getChild(i);

		if (pChild != nullptr)
		{
			char const* const typeAttrib = pChild->getAttr(s_typeAttrib);

			if (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoDspBus") == 0)
			{
				char const* const szNameValue = pChild->getAttr(s_nameAttrib);
				CItem* const pItem = CreateItem(szNameValue, EItemType::Bus, m_pBusesFolder, EItemFlags::None);
			}
			else if (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoDspSettingSnapshot") == 0)
			{
				char const* const szNameValue = pChild->getAttr(s_nameAttrib);
				CItem* const pItem = CreateItem(szNameValue, EItemType::Snapshot, m_pSnapShotsFolder, EItemFlags::None);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadWorkUnits(string const& folderPath, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "*", &fd);

	if (handle != -1)
	{
		do
		{
			string const fileName = fd.name;

			if ((fileName != ".") && (fileName != "..") && !fileName.empty())
			{
				string const filePath = folderPath + "/" + fileName;
				LoadWorkUnitFile(filePath, parent);

			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadWorkUnitFile(string const& folderPath, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*.atmcunit", &fd);

	if (handle != -1)
	{
		do
		{
			string fileName = fd.name;
			XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(folderPath + "/" + fileName);

			if (pRoot != nullptr)
			{
				PathUtil::RemoveExtension(fileName);
				XmlNodeRef const pWorkUnitNode = FindNodeByAttributeValue(pRoot, fileName);

				if (pWorkUnitNode != nullptr)
				{
					CItem* const pWorkUnit = CreateItem(fileName, EItemType::WorkUnit, &parent, EItemFlags::IsContainer);
					XmlNodeRef const pCueSheetFolderNode = FindNodeByAttributeValue(pRoot, "CueSheetFolder");

					if (pCueSheetFolderNode != nullptr)
					{
						CItem* const pCueSheetFolder = CreateItem("CueSheetFolder", EItemType::FolderCueSheet, pWorkUnit, EItemFlags::IsContainer);
						ParseWorkUnitFile(pCueSheetFolderNode, *pCueSheetFolder);
					}
				}
			}

			break;
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::ParseWorkUnitFile(XmlNodeRef const pRoot, CItem& parent)
{
	int const numChildren = pRoot->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pRoot->getChild(i);

		if (pChild != nullptr)
		{
			char const* const szNameValue = pChild->getAttr(s_nameAttrib);

			if (pChild->haveAttr("CueID"))
			{
				CreateItem(szNameValue, EItemType::Cue, &parent, EItemFlags::None);
			}
			else if (pChild->haveAttr("AwbHash"))
			{
				CItem* const pItem = CreateItem(szNameValue, EItemType::CueSheet, &parent, EItemFlags::IsContainer);
				ParseWorkUnitFile(pChild, *pItem);
			}
			else if (_stricmp(pChild->getAttr(s_typeAttrib), "CriMw.CriAtomCraft.AcCore.AcOoCueSheetSubFolder") == 0)
			{
				CItem* const pItem = CreateItem(szNameValue, EItemType::FolderCueSheet, &parent, EItemFlags::IsContainer);
				ParseWorkUnitFile(pChild, *pItem);
			}
			else if (_stricmp(pChild->getAttr(s_typeAttrib), "CriMw.CriAtomCraft.AcCore.AcOoCueFolder") == 0)
			{
				CItem* const pItem = CreateItem(szNameValue, EItemType::FolderCue, &parent, EItemFlags::IsContainer);
				ParseWorkUnitFile(pChild, *pItem);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadBinaries(string const& folderPath, bool const isLocalized, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*.acb", &fd);

	if (handle != -1)
	{
		EItemFlags const flags = isLocalized ? EItemFlags::IsLocalized : EItemFlags::None;

		do
		{
			string const fileName = fd.name;

			if (_stricmp(PathUtil::GetExt(fileName), "acb") == 0)
			{
				string const filePath = folderPath + "/" + fileName;
				EPakStatus const pakStatus = pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_OnDisk) ? EPakStatus::OnDisk : EPakStatus::None;

				CreateItem(fileName, EItemType::Binary, &parent, flags, pakStatus, filePath);
			}

		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::RemoveEmptyFolder(CItem* const pFolder)
{
	if (pFolder->GetNumChildren() == 0)
	{
		m_rootItem.RemoveChild(pFolder);
		ItemCache::const_iterator const it(m_itemCache.find(pFolder->GetId()));

		if (it != m_itemCache.end())
		{
			m_itemCache.erase(it);
		}

		delete pFolder;
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace ACE
