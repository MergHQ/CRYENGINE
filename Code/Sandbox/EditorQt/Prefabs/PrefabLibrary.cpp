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
		CRY_ASSERT(GetItemCount() <= 1, "Library %s has more than one prefab item", m_name.c_str());

		CBaseLibraryItem::SerializeContext ctx(root, bLoading);
		GetItem(0)->Serialize(ctx);
	}
}

void CPrefabLibrary::AddItem(IDataBaseItem* item, bool bRegister /* = true */)
{
	CRY_ASSERT(!GetItemCount(), "Library %s already has one prefab item", m_name.c_str());

	//make sure we only have one item in this library
	if (!GetItemCount())
	{
		CBaseLibrary::AddItem(item, bRegister);
	}
}

void CPrefabLibrary::UpdatePrefabObjects()
{
	CRY_ASSERT(GetItemCount() <= 1, "Library %s has more than one prefab item", m_name.c_str());

	if (m_items.size())
	{
		CPrefabItem* pPrefabItem = (CPrefabItem*)&*m_items[0];
		pPrefabItem->UpdateObjects();
	}
}
