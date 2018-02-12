// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplUtils.h"

#include <CryAudio/IAudioSystem.h>
#include <CrySystem/File/CryFile.h>
#include <CryString/CryPath.h>

namespace ACE
{
namespace Fmod
{
namespace Utils
{
//////////////////////////////////////////////////////////////////////////
CID GetId(EImplItemType const type, string const& name, CImplItem* const pParent, CImplItem const& rootItem)
{
	string const fullname = Utils::GetTypeName(type) + Utils::GetPathName(pParent, rootItem) + CRY_NATIVE_PATH_SEPSTR + name;
	return CryAudio::StringToId(fullname.c_str());
}

//////////////////////////////////////////////////////////////////////////
string GetPathName(CImplItem const* const pImplItem, CImplItem const& rootItem)
{
	string pathName = "";
	auto const editorFolderType = static_cast<ItemType>(EImplItemType::EditorFolder);

	if (pImplItem != nullptr)
	{
		string fullname = pImplItem->GetName();
		IImplItem const* pParent = pImplItem->GetParent();

		while ((pParent != nullptr) && (pParent->GetType() != editorFolderType) && (pParent != &rootItem))
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
string GetTypeName(EImplItemType const type)
{
	string name = "";

	switch (type)
	{
	case EImplItemType::Folder:
		name = "folder:";
		break;
	case EImplItemType::Event:
		name = "event:";
		break;
	case EImplItemType::Parameter:
		name = "parameter:";
		break;
	case EImplItemType::Snapshot:
		name = "snapshot:";
		break;
	case EImplItemType::Bank:
		name = "bank:";
		break;
	case EImplItemType::Return:
		name = "return:";
		break;
	case EImplItemType::VCA:
		name = "vca:";
		break;
	case EImplItemType::MixerGroup:
		name = "group:";
		break;
	case EImplItemType::EditorFolder:
		name = "editorfolder:";
		break;
	default:
		name = "";
		break;
	}

	return name;
}
} // namespace Utils
} // namespace Fmod
} // namespace ACE
