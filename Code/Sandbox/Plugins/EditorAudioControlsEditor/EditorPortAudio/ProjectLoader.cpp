// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ProjectLoader.h"

#include "Impl.h"
#include "Utils.h"

#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryAudioImplPortAudio/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void SetParentPakStatus(CItem* pParent, EPakStatus const pakStatus)
{
	while (pParent != nullptr)
	{
		pParent->SetPakStatus(pParent->GetPakStatus() | pakStatus);
		pParent = static_cast<CItem*>(pParent->GetParent());
	}
}

//////////////////////////////////////////////////////////////////////////
CProjectLoader::CProjectLoader(string const& assetsPath, string const& localizedAssetsPath, CItem& rootItem, ItemCache& itemCache, CImpl const& impl)
	: m_itemCache(itemCache)
	, m_impl(impl)
{
	LoadFolder(assetsPath, "", false, rootItem);
	LoadFolder(localizedAssetsPath, "", true, rootItem);
}

//////////////////////////////////////////////////////////////////////////
void CProjectLoader::LoadFolder(string const& assetsPath, string const& folderPath, bool const isLocalized, CItem& parent)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(assetsPath + "/" + folderPath + "/*.*", &fd);

	if (handle != -1)
	{
		EItemFlags const flags = isLocalized ? EItemFlags::IsLocalized : EItemFlags::None;

		do
		{
			string const name = fd.name;

			if ((name != ".") && (name != "..") && !name.empty())
			{
				if (fd.attrib & _A_SUBDIR)
				{
					if (folderPath.empty())
					{
						LoadFolder(assetsPath, name, isLocalized, *CreateItem(assetsPath, name, folderPath, EItemType::Folder, parent, flags));
					}
					else
					{
						LoadFolder(assetsPath, folderPath + "/" + name, isLocalized, *CreateItem(assetsPath, name, folderPath, EItemType::Folder, parent, flags));
					}
				}
				else
				{
					string::size_type const posExtension = name.rfind('.');

					if (posExtension != string::npos)
					{
						char const* const szExtension = name.data() + posExtension + 1;

						for (auto const& pair : CryAudio::Impl::PortAudio::g_supportedExtensions)
						{
							if (_stricmp(szExtension, pair.first) == 0)
							{
								// Create the event with the same name as the file
								CreateItem(assetsPath, name, folderPath, EItemType::Event, parent, flags);
								break;
							}
						}
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
CItem* CProjectLoader::CreateItem(string const& assetsPath, string const& name, string const& path, EItemType const type, CItem& parent, EItemFlags flags)
{
	bool const isLocalized = (flags& EItemFlags::IsLocalized) != EItemFlags::None;
	ControlId const id = Utils::GetId(type, name, path, isLocalized);
	auto pItem = static_cast<CItem*>(m_impl.GetItem(id));

	if (pItem == nullptr)
	{
		string filePath = assetsPath + "/";

		if (path.empty())
		{
			filePath += name;
		}
		else
		{
			filePath += (path + "/" + name);
		}

		EPakStatus pakStatus = EPakStatus::None;

		if (gEnv->pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_InPak))
		{
			pakStatus |= EPakStatus::InPak;
		}

		if (gEnv->pCryPak->IsFileExist(filePath.c_str(), ICryPak::eFileLocation_OnDisk))
		{
			pakStatus |= EPakStatus::OnDisk;
		}

		if (type == EItemType::Event)
		{
			SetParentPakStatus(&parent, pakStatus);
		}
		else if (type == EItemType::Folder)
		{
			flags |= EItemFlags::IsContainer;
		}

		pItem = new CItem(name, id, type, path, flags, pakStatus, filePath);

		parent.AddChild(pItem);
		m_itemCache[id] = pItem;
	}

	return pItem;
}
} // namespace PortAudio
} // namespace Impl
} // namespace ACE
