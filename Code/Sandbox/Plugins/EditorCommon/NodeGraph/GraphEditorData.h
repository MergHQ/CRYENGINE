// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include <unordered_map>
#include <CrySerialization/IArchive.h>

#include "NodeEditorData.h"
#include "GroupEditorData.h"
#include "CommentEditorData.h"

namespace CryGraphEditor {

using CNodeEditorDataByIndex    = std::vector<CNodeEditorData*>;
using CGroupEditorDataByIndex   = std::vector<CGroupEditorData*>;
using CCommentEditorDataByIndex = std::vector<CCommentEditorData*>;

class EDITOR_COMMON_API CGraphEditorData
{
public:
	virtual ~CGraphEditorData() {}
	virtual void                Serialize(Serialization::IArchive& archive);

	virtual uint32_t            GetNodeEditorDataCount() const;
	virtual CNodeEditorData*    CreateNodeEditorData(const QVariant& id);
	virtual void                RemoveNodeEditorDataById(const QVariant& id);
	virtual CNodeEditorData*    GetNodeEditorDataById(const QVariant& id);
	virtual CNodeEditorData*    GetNodeEditorDataByIndex(unsigned index);

	virtual uint32_t            GetGroupEditorDataCount() const;
	virtual CGroupEditorData*   CreateGroupEditorData(const QVariant& id);
	virtual void                RemoveGroupEditorDataById(const QVariant& id);
	virtual CGroupEditorData*   GetGroupEditorDataById(const QVariant& id);
	virtual CGroupEditorData*   GetGroupEditorDataByIndex(unsigned index);

	virtual uint32_t            GetCommentEditorDataCount() const;
	virtual CCommentEditorData* CreateCommentEditorData(const QVariant& id);
	virtual void                RemoveCommentEditorDataById(const QVariant& id);
	virtual CCommentEditorData* GetCommentEditorDataById(const QVariant& id);
	virtual CCommentEditorData* GetCommentEditorDataByIndex(unsigned index);

protected:
	CNodeEditorDataByIndex      m_nodeEditorDataByIndex;
	CGroupEditorDataByIndex     m_groupEditorDataByIndex;
	CCommentEditorDataByIndex   m_commentEditorDataByIndex;
};

} //namespace CryGraphEditor
