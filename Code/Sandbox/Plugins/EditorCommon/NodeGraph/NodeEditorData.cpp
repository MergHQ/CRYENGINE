// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "NodeEditorData.h"

namespace CryGraphEditor {

CNodeEditorData::CNodeEditorData(QVariant id)
	: CAbstractEditorData(id)
{
}

void CNodeEditorData::Serialize(Serialization::IArchive& archive)
{
	CAbstractEditorData::Serialize(archive);
	archive(m_pos, "pos");
}

} // CryGraphEditor
