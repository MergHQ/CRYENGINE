// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CommentEditorData.h"

namespace CryGraphEditor {

CCommentEditorData::CCommentEditorData(QVariant id)
	: CAbstractEditorData(id)
{
}

void CCommentEditorData::Serialize(Serialization::IArchive& archive)
{
	CAbstractEditorData::Serialize(archive);

	archive(m_pos,  "pos");
	archive(m_text, "text");
}

} // namespace CAbstractCommentData
