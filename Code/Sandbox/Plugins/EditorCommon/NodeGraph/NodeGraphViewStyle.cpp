// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NodeGraphViewStyle.h"

#include "NodeWidgetStyle.h"
#include "GroupWidgetStyle.h"
#include "CommentWidgetStyle.h"
#include "ConnectionWidgetStyle.h"
#include "NodePinWidgetStyle.h"

#include "NodeGraphView.h"
#include <CryCore/CryCrc32.h>

namespace CryGraphEditor {

CNodeGraphViewStyle::CNodeGraphViewStyle(const char* szStyleId)
	: QWidget()
{
	setObjectName(szStyleId);

	m_selectionColor = QColor(97, 172, 237);
	m_highlightColor = QColor(140, 140, 140);

	m_gridBackgroundColor = QColor(0x5F, 0x5F, 0x5F);
	m_gridSegmentLineColor = QColor(0x46, 0x46, 0x46);
	m_gridSegmentLineWidth = 1.f;
	m_gridSegmentSize = 140.f;

	m_gridSubSegmentLineColor = QColor(0x70, 0x70, 0x70);
	m_gridSubSegmentCount = 8;
	m_gridSubSegmentLineWidth = 1.f;

	new CNodeWidgetStyle("Node", *this);
	new CConnectionWidgetStyle("Connection", *this);
	new CNodePinWidgetStyle("Pin", *this);
}

void CNodeGraphViewStyle::RegisterNodeWidgetStyle(CNodeWidgetStyle* pStyle)
{
	CRY_ASSERT_MESSAGE(pStyle->GetId(), "StyleId must be non-zero!");
	if (pStyle->GetId())
	{
		const uint32 styleIdHash = pStyle->GetIdHash();
		const bool iconExists = (m_nodeWidgetStylesById.find(styleIdHash) != m_nodeWidgetStylesById.end());

		if (iconExists == false)
		{
			pStyle->SetParent(this);
			m_nodeWidgetStylesById[styleIdHash] = pStyle;
		}
		else
		{
			auto result = m_nodeWidgetStylesById.find(styleIdHash);
			const stack_string resultStyleId = result->second->GetId();
			if (pStyle->GetId() == resultStyleId)
			{
				//CRY_ASSERT_MESSAGE(false, "Style id already exists.");
			}
			else
			{
				CRY_ASSERT_MESSAGE(false, "Hash collison of style id '%s' and '%s'", pStyle->GetId(), resultStyleId.c_str());
			}
		}
	}
}

const CNodeWidgetStyle* CNodeGraphViewStyle::GetNodeWidgetStyle(const char* styleId) const
{
	CRY_ASSERT(styleId);
	const StyleIdHash styleIdHash = CCrc32::Compute(styleId);
	auto result = m_nodeWidgetStylesById.find(styleIdHash);
	if (result != m_nodeWidgetStylesById.end())
	{
		return result->second;
	}
	return nullptr;
}

void CNodeGraphViewStyle::RegisterGroupWidgetStyle(CGroupWidgetStyle* pStyle)
{
	CRY_ASSERT_MESSAGE(pStyle->GetId(), "StyleId must be non-zero!");
	if (pStyle->GetId())
	{
		const uint32 styleIdHash = pStyle->GetIdHash();
		const bool iconExists = (m_groupWidgetStylesById.find(styleIdHash) != m_groupWidgetStylesById.end());

		if (iconExists == false)
		{
			pStyle->SetParent(this);
			m_groupWidgetStylesById[styleIdHash] = pStyle;
		}
		else
		{
			auto result = m_groupWidgetStylesById.find(styleIdHash);
			const stack_string resultStyleId = result->second->GetId();
			if (pStyle->GetId() == resultStyleId)
			{
				CRY_ASSERT_MESSAGE(false, "Style id already exists.");
			}
			else
			{
				CRY_ASSERT_MESSAGE(false, "Hash collison of style id '%s' and '%s'", pStyle->GetId(), resultStyleId.c_str());
			}
		}
	}
}

const CGroupWidgetStyle* CNodeGraphViewStyle::GetGroupWidgetStyle(const char* styleId) const
{
	CRY_ASSERT_MESSAGE(styleId, "StyleId must be non-zero!");
	const StyleIdHash styleIdHash = CCrc32::Compute(styleId);
	auto result = m_groupWidgetStylesById.find(styleIdHash);
	if (result != m_groupWidgetStylesById.end())
	{
		return result->second;
	}
	return nullptr;
}

void CNodeGraphViewStyle::RegisterCommentWidgetStyle(CCommentWidgetStyle* pStyle)
{
	CRY_ASSERT_MESSAGE(pStyle->GetId(), "StyleId must be non-zero!");
	if (pStyle->GetId())
	{
		const uint32 styleIdHash = pStyle->GetIdHash();
		const bool iconExists = (m_commentWidgetStylesById.find(styleIdHash) != m_commentWidgetStylesById.end());

		if (iconExists == false)
		{
			pStyle->SetParent(this);
			m_commentWidgetStylesById[styleIdHash] = pStyle;
		}
		else
		{
			auto result = m_commentWidgetStylesById.find(styleIdHash);
			const stack_string resultStyleId = result->second->GetId();
			if (pStyle->GetId() == resultStyleId)
			{
				//CRY_ASSERT_MESSAGE(false, "Style id already exists.");
			}
			else
			{
				CRY_ASSERT_MESSAGE(false, "Hash collison of style id '%s' and '%s'", pStyle->GetId(), resultStyleId.c_str());
			}
		}
	}
}

const CCommentWidgetStyle* CNodeGraphViewStyle::GetCommentWidgetStyle(const char* styleId) const
{
	CRY_ASSERT_MESSAGE(styleId, "StyleId must be non-zero!");
	const StyleIdHash styleIdHash = CCrc32::Compute(styleId);
	auto result = m_commentWidgetStylesById.find(styleIdHash);
	if (result != m_commentWidgetStylesById.end())
	{
		return result->second;
	}
	return nullptr;
}

void CNodeGraphViewStyle::RegisterConnectionWidgetStyle(CConnectionWidgetStyle* pStyle)
{
	CRY_ASSERT_MESSAGE(pStyle->GetId(), "StyleId must be non-zero!");
	if (pStyle->GetId())
	{
		const StyleIdHash styleIdHash = pStyle->GetIdHash();
		const bool iconExists = (m_connectionWidgetStylesById.find(styleIdHash) != m_connectionWidgetStylesById.end());

		if (iconExists == false)
		{
			pStyle->SetParent(this);
			m_connectionWidgetStylesById[styleIdHash] = pStyle;
		}
		else
		{
			auto result = m_connectionWidgetStylesById.find(styleIdHash);
			const stack_string resultStyleId = result->second->GetId();
			if (pStyle->GetId() == resultStyleId)
			{
				CRY_ASSERT_MESSAGE(false, "Style id already exists.");
			}
			else
			{
				CRY_ASSERT_MESSAGE(false, "Hash collison of style id '%s' and '%s'", pStyle->GetId(), resultStyleId.c_str());
			}
		}
	}
}

const CConnectionWidgetStyle* CNodeGraphViewStyle::GetConnectionWidgetStyle(const char* styleId) const
{
	CRY_ASSERT(styleId);
	const StyleIdHash styleIdHash = CCrc32::Compute(styleId);
	auto result = m_connectionWidgetStylesById.find(styleIdHash);
	if (result != m_connectionWidgetStylesById.end())
	{
		return result->second;
	}
	return nullptr;
}

void CNodeGraphViewStyle::RegisterPinWidgetStyle(CNodePinWidgetStyle* pStyle)
{
	CRY_ASSERT_MESSAGE(pStyle->GetId(), "StyleId must be non-zero!");
	if (pStyle->GetId())
	{
		const StyleIdHash styleIdHash = pStyle->GetIdHash();
		const bool iconExists = (m_pinWidgetStylesById.find(styleIdHash) != m_pinWidgetStylesById.end());

		if (iconExists == false)
		{
			pStyle->SetParent(this);
			m_pinWidgetStylesById[styleIdHash] = pStyle;
		}
		else
		{
			auto result = m_pinWidgetStylesById.find(styleIdHash);
			const stack_string resultStyleId = result->second->GetId();
			if (pStyle->GetId() == resultStyleId)
			{
				CRY_ASSERT_MESSAGE(false, "Style id already exists.");
			}
			else
			{
				CRY_ASSERT_MESSAGE(false, "Hash collison of style id '%s' and '%s'", pStyle->GetId(), resultStyleId.c_str());
			}
		}
	}
}

const CNodePinWidgetStyle* CNodeGraphViewStyle::GetPinWidgetStyle(const char* styleId) const
{
	CRY_ASSERT(styleId);
	const StyleIdHash styleIdHash = CCrc32::Compute(styleId);
	auto result = m_pinWidgetStylesById.find(styleIdHash);
	if (result != m_pinWidgetStylesById.end())
	{
		return result->second;
	}
	return nullptr;
}

}
