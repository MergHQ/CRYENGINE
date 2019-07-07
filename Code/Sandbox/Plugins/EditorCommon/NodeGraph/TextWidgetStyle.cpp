// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "TextWidgetStyle.h"

#include "NodeWidgetStyle.h"
#include "NodeGraphView.h"

#include <QApplication>

namespace CryGraphEditor {

CTextWidgetStyle::CTextWidgetStyle(CNodeGraphViewStyleItem& viewItemStyle)
	: QWidget(&viewItemStyle)
	, m_viewItemStyle(viewItemStyle)
{
	m_alignment = Qt::AlignLeft | Qt::AlignVCenter;

	m_textFont = QApplication::font();
	m_textColor = QColor(94, 94, 94);

	//
	m_height = 24;
	m_margins = QMargins(7, 1, -7, 1);
}

}
