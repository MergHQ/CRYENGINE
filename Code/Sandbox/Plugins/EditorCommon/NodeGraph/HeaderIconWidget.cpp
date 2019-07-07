// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "HeaderIconWidget.h"

#include "HeaderWidget.h"
#include "NodeGraphViewStyle.h"
#include "NodeWidget.h"

#include <QtUtil.h>

namespace CryGraphEditor {

CHeaderIconWidget::CHeaderIconWidget(CNodeGraphViewGraphicsWidget& viewWidget)
	: m_viewWidget(viewWidget)
	, m_pPixmap(nullptr)
	, m_viewItem(*viewWidget.GetAbstractItem())
{
	setAcceptHoverEvents(true);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setMaximumSize(QSizeF(16.0, 16.0) /*CNodeStyle::GetHeaderIconSize()*/);
	setMinimumSize(QSizeF(16.0, 16.0) /*CNodeStyle::GetHeaderIconSize()*/);
	setPreferredSize(QSizeF(16.0, 16.0) /*CNodeStyle::GetHeaderIconSize()*/);
}

CAbstractNodeGraphViewModelItem& CHeaderIconWidget::GetViewItem() const
{
	// as now CNodeGraphViewGraphicsWidget::GetAbstractItem() is virtual
	// it is cannot invoked from here; because this method could be invoked
	// from CNodeWidget destruction frame, which leads to pure virtual call
	return m_viewItem;
}

void CHeaderIconWidget::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
	if (m_pPixmap)
	{
		pPainter->drawPixmap(boundingRect(), *m_pPixmap, m_pPixmap->rect());
	}
}

void CHeaderIconWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* pEvent)
{
	OnMouseEnter();
}

void CHeaderIconWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent)
{
	OnMouseLeave();
}

void CHeaderIconWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent)
{
	OnClicked();
}

QRectF CHeaderIconWidget::boundingRect() const
{
	return QRectF(QPointF(0, 0), QSizeF(16.0, 16.0) /*CNodeStyle::GetHeaderIconSize()*/);
}

QSizeF CHeaderIconWidget::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
	return QSizeF(16.0, 16.0) /*CNodeStyle::GetHeaderIconSize()*/;
}

}
