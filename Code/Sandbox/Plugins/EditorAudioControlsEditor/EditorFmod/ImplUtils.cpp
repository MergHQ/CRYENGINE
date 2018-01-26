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
CID GetId(EImpltemType const type, string const& name, CImplItem* const pParent, CImplItem const& rootControl)
{
	string const fullname = Utils::GetTypeName(type) + Utils::GetPathName(pParent, rootControl) + CRY_NATIVE_PATH_SEPSTR + name;
	return CryAudio::StringToId(fullname.c_str());
}

//////////////////////////////////////////////////////////////////////////
string GetPathName(CImplItem const* const pImplItem, CImplItem const& rootControl)
{
	string pathName = "";
	auto const editorFolderType = static_cast<ItemType>(EImpltemType::EditorFolder);

	if (pImplItem != nullptr)
	{
		string fullname = pImplItem->GetName();
		CImplItem const* pParent = pImplItem->GetParent();

		while ((pParent != nullptr) && (pParent->GetType() != editorFolderType) && (pParent != &rootControl))
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
string GetTypeName(EImpltemType const type)
{
	string name = "";

	switch (type)
	{
	case EImpltemType::Folder:
		name = "folder:";
		break;
	case EImpltemType::Event:
		name = "event:";
		break;
	case EImpltemType::Parameter:
		name = "parameter:";
		break;
	case EImpltemType::Snapshot:
		name = "snapshot:";
		break;
	case EImpltemType::Bank:
		name = "bank:";
		break;
	case EImpltemType::Return:
		name = "return:";
		break;
	case EImpltemType::VCA:
		name = "vca:";
		break;
	case EImpltemType::MixerGroup:
		name = "group:";
		break;
	case EImpltemType::EditorFolder:
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
