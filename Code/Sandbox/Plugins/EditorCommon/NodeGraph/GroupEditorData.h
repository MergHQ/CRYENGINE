// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "AbstractEditorData.h"

#include <CryExtension/CryGUID.h>
#include <CrySerialization/Math.h>

namespace CryGraphEditor {

class EDITOR_COMMON_API CGroupEditorData : public CAbstractEditorData
{
public:
	class CItem
	{
	public:
		Vec2     m_pivot;
		void     Serialize(Serialization::IArchive& archive);
	};
	using Items = std::map<CryGUID, CItem>;

public:
	CGroupEditorData(QVariant id = QVariant());

	void         SetAA(const Vec2& aa) { m_aa = aa; }
	Vec2         GetAA() const { return m_aa; }

	void         SetBB(const Vec2& bb) { m_bb = bb; }
	Vec2         GetBB() const { return m_bb; }

	void         SetPos(const Vec2& pos) { m_pos = pos; }
	Vec2         GetPos() const { return m_pos; }

	void         SetName(const string& name) { m_name = name; }
	string       GetName() const { return m_name; }

	Items&       GetItems() { return m_items; }
	const Items& GetItems() const { return m_items; }

	virtual void Serialize(Serialization::IArchive& archive);	

private:
	Vec2         m_aa;
	Vec2         m_bb;
	Vec2         m_pos;
	string       m_name;
	Items        m_items;
};
using CGroupItems = CGroupEditorData::Items;

EDITOR_COMMON_API inline bool Serialize(Serialization::IArchive& archive, CGroupEditorData*& data, const char* name, const char* label)
{
	if (archive.isInput() && (data == nullptr))
	{
		data = new CGroupEditorData;
	}

	return archive(*data, name, label);
}

} // namespace CryGraphEditor
