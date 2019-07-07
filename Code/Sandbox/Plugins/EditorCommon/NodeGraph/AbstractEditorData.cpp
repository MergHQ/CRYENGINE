// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "AbstractEditorData.h"

namespace CryGraphEditor {

CAbstractEditorData::CAbstractEditorData(QVariant id)
	: m_id(id)
{
}

void CAbstractEditorData::Serialize(Serialization::IArchive& archive)
{
	CryGUID guid = m_id.value<CryGUID>();
	archive(guid, "id");
	m_id = QVariant::fromValue(guid);
}

} //CryGraphEditor
