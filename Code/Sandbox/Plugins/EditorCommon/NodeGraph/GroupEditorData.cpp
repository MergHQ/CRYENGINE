// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GroupEditorData.h"

namespace CryGraphEditor {

void CGroupEditorData::CItem::Serialize(Serialization::IArchive& archive)
{
	archive(m_pivot, "pivot");
}

CGroupEditorData::CGroupEditorData(QVariant id)
	: CAbstractEditorData(id)
{
}

void CGroupEditorData::Serialize(Serialization::IArchive& archive)
{
	CAbstractEditorData::Serialize(archive);

	archive(m_aa,    "aa");
	archive(m_bb,    "bb");
	archive(m_pos,   "pos");
	archive(m_name,  "name");
	archive(m_items, "items");
}

} // namespace CryGraphEditor