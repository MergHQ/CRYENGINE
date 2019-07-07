// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "AbstractEditorData.h"
#include <CrySerialization/Math.h>
#include <CrySandbox/CrySignal.h>

namespace CryGraphEditor {

class EDITOR_COMMON_API CNodeEditorData : public CAbstractEditorData
{
public:
	CCrySignal<void()> SignalDataChanged;

public:
	CNodeEditorData(QVariant id = QVariant());

	void         SetPos(const Vec2& pos) { m_pos = pos; }
	Vec2         GetPos() const { return m_pos; }

	virtual void Serialize(Serialization::IArchive& archive);

protected:
	Vec2         m_pos;
};

EDITOR_COMMON_API inline bool Serialize(Serialization::IArchive& archive, CNodeEditorData*& data, const char* name, const char* label)
{
	if (archive.isInput() && (data == nullptr))
	{
		data = new CNodeEditorData;
	}

	return archive(*data, name, label);
}

} //namespace CryGraphEditor
