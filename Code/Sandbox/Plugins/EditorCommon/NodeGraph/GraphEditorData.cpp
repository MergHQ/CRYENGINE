// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GraphEditorData.h"

namespace CryGraphEditor {

void CGraphEditorData::Serialize(Serialization::IArchive& archive)
{
	archive(m_nodeEditorDataByIndex,    "NodeEditorData",    "node");
	archive(m_groupEditorDataByIndex,   "GroupEditorData",   "group");
	archive(m_commentEditorDataByIndex, "CommentEditorData", "comment");
}

uint32_t CGraphEditorData::GetNodeEditorDataCount() const
{
	return m_nodeEditorDataByIndex.size();
}

CNodeEditorData* CGraphEditorData::CreateNodeEditorData(const QVariant& id)
{
	CRY_ASSERT_MESSAGE(std::find_if(m_nodeEditorDataByIndex.begin(), m_nodeEditorDataByIndex.end(), [id](const CNodeEditorData* data) 
	{	
		return data->GetId() == id;
	} 
	) == m_nodeEditorDataByIndex.end(), "Node editor data structure with the specified id already exists.");

	m_nodeEditorDataByIndex.push_back(new CNodeEditorData(id));
	return m_nodeEditorDataByIndex.back();
}

void CGraphEditorData::RemoveNodeEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_nodeEditorDataByIndex.begin(), m_nodeEditorDataByIndex.end(), [id](const CNodeEditorData* data) 
	{
		return data->GetId() == id;
	});

	if (it != m_nodeEditorDataByIndex.end())
	{		
		m_nodeEditorDataByIndex.erase(it);
		delete *it;
	}
}

CNodeEditorData* CGraphEditorData::GetNodeEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_nodeEditorDataByIndex.begin(), m_nodeEditorDataByIndex.end(), [id](const CNodeEditorData* data)
	{
		return data->GetId() == id;
	});

	if (it != m_nodeEditorDataByIndex.end())
	{
		return *it;
	}

	return nullptr;
}

CNodeEditorData* CGraphEditorData::GetNodeEditorDataByIndex(unsigned index)
{
	return m_nodeEditorDataByIndex[index];
}

uint32_t CGraphEditorData::GetGroupEditorDataCount() const
{
	return m_groupEditorDataByIndex.size();
}

CGroupEditorData* CGraphEditorData::CreateGroupEditorData(const QVariant& id)
{
	CRY_ASSERT_MESSAGE(std::find_if(m_groupEditorDataByIndex.begin(), m_groupEditorDataByIndex.end(), [id](const CGroupEditorData* data) 
	{ 
		return data->GetId() == id; 
	}
	) == m_groupEditorDataByIndex.end(), "Group editor data structure with the specified id already exists.");
	
	m_groupEditorDataByIndex.push_back(new CGroupEditorData(id));
	return m_groupEditorDataByIndex.back();
}

void CGraphEditorData::RemoveGroupEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_groupEditorDataByIndex.begin(), m_groupEditorDataByIndex.end(), [id](const CGroupEditorData* data)
	{
		return data->GetId() == id;
	});

	if (it != m_groupEditorDataByIndex.end())
	{
		m_groupEditorDataByIndex.erase(it);
		delete *it;
	}
}

CGroupEditorData* CGraphEditorData::GetGroupEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_groupEditorDataByIndex.begin(), m_groupEditorDataByIndex.end(), [id](const CGroupEditorData* data)
	{
		return data->GetId() == id;
	});

	if (it != m_groupEditorDataByIndex.end())
	{
		return *it;
	}

	return nullptr;
}

CGroupEditorData* CGraphEditorData::GetGroupEditorDataByIndex(unsigned index)
{
	return m_groupEditorDataByIndex[index];
}

uint32_t CGraphEditorData::GetCommentEditorDataCount() const
{
	return m_commentEditorDataByIndex.size();
}

CCommentEditorData* CGraphEditorData::CreateCommentEditorData(const QVariant& id)
{
	CRY_ASSERT_MESSAGE(std::find_if(m_commentEditorDataByIndex.begin(), m_commentEditorDataByIndex.end(), [id](const CCommentEditorData* data) 
	{ 
		return data->GetId() == id; 
	}
	) == m_commentEditorDataByIndex.end(), "Comment editor data structure with the specified id already exists.");

	m_commentEditorDataByIndex.push_back(new CCommentEditorData(id));
	return m_commentEditorDataByIndex.back();
}

void CGraphEditorData::RemoveCommentEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_commentEditorDataByIndex.begin(), m_commentEditorDataByIndex.end(), [id](const CCommentEditorData* data)
	{
		return data->GetId() == id;
	});

	if (it != m_commentEditorDataByIndex.end())
	{
		m_commentEditorDataByIndex.erase(it);
		delete *it;
	}
}

CCommentEditorData* CGraphEditorData::GetCommentEditorDataById(const QVariant& id)
{
	auto it = std::find_if(m_commentEditorDataByIndex.begin(), m_commentEditorDataByIndex.end(), [id](const CCommentEditorData* data)
	{
		return data->GetId() == id;
	});

	if (it != m_commentEditorDataByIndex.end())
	{
		return *it;
	}

	return nullptr;
}

CCommentEditorData* CGraphEditorData::GetCommentEditorDataByIndex(unsigned index)
{
	return m_commentEditorDataByIndex[index];
}

} // namespace CryGraphEditor
