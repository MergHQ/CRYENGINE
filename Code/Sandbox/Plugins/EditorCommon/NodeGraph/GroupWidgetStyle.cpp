// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GroupWidgetStyle.h"

#include "HeaderWidgetStyle.h"

namespace CryGraphEditor {

CGroupWidgetStyle::CGroupWidgetStyle(const char* szStyleId, CNodeGraphViewStyle& viewStyle)
	: CNodeGraphViewStyleItem(szStyleId)
{
	m_minimalExtents = QPointF(400, 171);
	m_borderColor = QColor(84, 84, 84);
	m_borderWidth = 1;
	m_backgroundColor = QColor(32, 32, 32);
	m_margins = QMargins(6, 3, 6, 6);

	m_pendingBorderColor = QColor(0xFF, 0xFF, 0xFF, 0x80);
	m_pendingBorderWidth = 2;
	m_pendingBackgroundColor = QColor(0x00, 0x00, 0x00, 0x33);
	viewStyle.RegisterGroupWidgetStyle(this);
}

}
