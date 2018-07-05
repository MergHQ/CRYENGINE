// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "AbstractEditorData.h"
#include <CrySerialization/Math.h>

namespace CryGraphEditor {

class EDITOR_COMMON_API CCommentEditorData : public CAbstractEditorData
{
public:
	CCommentEditorData(QVariant id = QVariant());

	void         SetPos(const Vec2& pos) { m_pos = pos; }
	Vec2         GetPos() const { return m_pos; }

	void         SetText(const string& text) { m_text = text; }
	string&      GetText() { return m_text; }

	virtual void Serialize(Serialization::IArchive& archive);

private:
	Vec2         m_pos;
	string       m_text;
};

EDITOR_COMMON_API inline bool Serialize(Serialization::IArchive& archive, CCommentEditorData*& data, const char* name, const char* label)
{
	if (archive.isInput() && (data == nullptr))
	{
		data = new CCommentEditorData;
	}

	return archive(*data, name, label);
}

} //namespace CryEditorData
