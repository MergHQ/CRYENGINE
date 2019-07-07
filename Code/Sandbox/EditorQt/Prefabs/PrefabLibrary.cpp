// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PrefabLibrary.h"
#include "PrefabItem.h"

bool CPrefabLibrary::Save()
{
	return SaveLibrary("Prefab");
}

bool CPrefabLibrary::Load(const string& filename)
{
	if (filename.IsEmpty())
		return false;

	SetFilename(filename);
	SetName(PathUtil::RemoveExtension(filename));

	XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename);
	if (!root)
		return false;

	Serialize(root, true);

	return true;
}

void CPrefabLibrary::Serialize(XmlNodeRef& root, bool bLoading)
{
	if (bLoading)
	{
		// Loading.
		RemoveAllItems();

		if (stricmp(root->getTag(), "Prefab") == 0)
		{
			CBaseLibraryItem* pItem = new CPrefabItem;
			AddItem(pItem, false);

			CBaseLibraryItem::SerializeContext ctx(root, bLoading);
			pItem->Serialize(ctx);

			SetModified(false);

		}
	}
	else if(GetItemCount()) // saving
	{
		CBaseLibraryItem::SerializeContext ctx(root, bLoading);
		GetItem(0)->Serialize(ctx);
	}

}

void CPrefabLibrary::UpdatePrefabObjects()
{
	for (int i = 0, iItemSize(m_items.size()); i < iItemSize; ++i)
	{
		CPrefabItem* pPrefabItem = (CPrefabItem*)&*m_items[i];
		pPrefabItem->UpdateObjects();
	}
}
