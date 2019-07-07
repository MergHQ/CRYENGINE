// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NodeWidgetStyle.h"

#include "HeaderWidgetStyle.h"
#include "NodeContentWidgetStyle.h"
#include "NodeInfoWidgetStyle.h"

namespace CryGraphEditor {

CNodeWidgetStyle::CNodeWidgetStyle(const char* szStyleId, CNodeGraphViewStyle& viewStyle)
	: CNodeGraphViewStyleItem(szStyleId)
{
	m_borderColor = QColor(84, 84, 84);
	m_borderWidth = 1;
	m_backgroundColor = QColor(32, 32, 32);
	m_margins = QMargins(6, 3, 6, 6);

	m_pContentStyle = CreateContentWidgetStyle();
	m_pInfoWidgetStyle = CreateInfoWidgetStyle();

	viewStyle.RegisterNodeWidgetStyle(this);
}

CNodeContentWidgetStyle* CNodeWidgetStyle::CreateContentWidgetStyle()
{
	return new CNodeContentWidgetStyle(*this);
}

CNodeInfoWidgetStyle* CNodeWidgetStyle::CreateInfoWidgetStyle()
{
	return new CNodeInfoWidgetStyle(*this);
}

}
