// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NodeGraphStyleItem.h"

#include "TextWidgetStyle.h"
#include "HeaderWidgetStyle.h"
#include "NodeGraphViewStyle.h"
#include <CryCore/CryCrc32.h>


namespace CryGraphEditor {

CNodeGraphViewStyleItem::CNodeGraphViewStyleItem(const char* szStyleId)
	: m_styleId(szStyleId)
	, m_styleIdHash(CCrc32::Compute(szStyleId))
	, m_pViewStyle(nullptr)
{
	CRY_ASSERT(szStyleId);
	setObjectName(szStyleId);

	m_pTextStyle   = CreateTextWidgetStyle();
	m_pHeaderStyle = CreateHeaderWidgetStyle();
}

CNodeGraphViewStyle* CNodeGraphViewStyleItem::GetViewStyle() const
{
	return m_pViewStyle;
}

void CNodeGraphViewStyleItem::SetParent(CNodeGraphViewStyle* pViewStyle)
{
	m_pViewStyle = pViewStyle;
	setParent(pViewStyle);
}

const QColor& CNodeGraphViewStyleItem::GetSelectionColor() const
{ 
	return GetViewStyle()->GetSelectionColor(); 
}

const QColor& CNodeGraphViewStyleItem::GetHighlightColor() const
{ 
	return GetViewStyle()->GetHighlightColor(); 
}

const QIcon& CNodeGraphViewStyleItem::GetTypeIcon() const
{
	return m_pHeaderStyle->GetNodeIconMenu();
}

CTextWidgetStyle* CNodeGraphViewStyleItem::CreateTextWidgetStyle()
{
	return new CTextWidgetStyle(*this);
}

CHeaderWidgetStyle* CNodeGraphViewStyleItem::CreateHeaderWidgetStyle()
{
	return new CHeaderWidgetStyle(*this);
}

}
