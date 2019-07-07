// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CommentWidget.h"

#include "TextWidget.h"
#include "NodeGraphView.h"
#include "CommentWidgetStyle.h"

#include <CryMath/Cry_Math.h>

#include <QPainter>
#include <QBrush>
#include <QPointF>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

namespace CryGraphEditor {

CCommentWidget::CCommentWidget(CAbstractCommentItem& item, CNodeGraphView& view)
	: CNodeGraphViewGraphicsWidget(view)
	, m_item(item)
	, m_pLayout(nullptr)
	, m_pStyle(GetView().GetStyle().GetCommentWidgetStyle(item.GetStyleId()))
{
	CRY_ASSERT(m_pStyle);
	if (!m_pStyle)
	{
		m_pStyle = GetView().GetStyle().GetCommentWidgetStyle("Comment");
	}

	m_item.SignalTextChanged.Connect(this, &CCommentWidget::OnItemTextChanged);
	m_item.SignalPositionChanged.Connect(this, &CCommentWidget::OnItemPositionChanged);

	m_pLayout = new QGraphicsLinearLayout();
	m_pLayout->setContentsMargins(4.0f, 4.0f, 0.0f, 0.0f);
	m_pLayout->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	m_pText = new CTextWidget(*this, *this);
	m_pText->SetMultiline(true);
	m_pText->SignalTextChanged.Connect([this](const QString& text) 
	{ 
		m_item.GetEditorData().SetText(text.toStdString().c_str());
		m_item.SignalTextChanged();
	});

	m_pLayout->addItem(m_pText);
	m_pLayout->setAlignment(m_pText, Qt::AlignCenter);

	setLayout(m_pLayout);
	SetDefaultZValue(1.0f - 0.01f);

	QPointF extents = m_pStyle->GetMinimalExtents();

	setMinimumWidth(extents.x());
	setMinimumHeight(extents.y());

	OnItemPositionChanged();
}

CCommentWidget::~CCommentWidget()
{

}

void CCommentWidget::DeleteLater()
{
	CNodeGraphViewGraphicsWidget::DeleteLater();
}

const CNodeGraphViewStyleItem& CCommentWidget::GetStyle() const
{
	return *m_pStyle;
}

CAbstractNodeGraphViewModelItem* CCommentWidget::GetAbstractItem() const
{ 
	return &m_item; 
}

void CCommentWidget::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
	if (isVisible())
	{
		pPainter->save();

		pPainter->setBrush(m_pStyle->GetBackgroundColor());
		pPainter->setPen(m_color);
		pPainter->drawRect(boundingRect());

		pPainter->setBrush(Qt::NoBrush);
		pPainter->setPen(QColor(255, 0, 0));

		pPainter->restore();
	}
}

QRectF CCommentWidget::boundingRect() const
{
	const QRectF geometryRect = geometry();
	return QRectF(0, 0, geometryRect.width(), geometryRect.height());
}

void CCommentWidget::mousePressEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::ButtonPress,
		pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnCommentMouseEvent(this, args);
}

void CCommentWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
	  EMouseEventReason::ButtonRelease,
	  pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
	  pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
	  pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnCommentMouseEvent(this, args);
}

void CCommentWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
	  EMouseEventReason::ButtonPressAndMove,
	  pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
	  pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
	  pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnCommentMouseEvent(this, args);
}

void CCommentWidget::moveEvent(QGraphicsSceneMoveEvent* pEvent)
{
	// TODO: Signal to grouped items.
}

void CCommentWidget::OnItemTextChanged()
{
	CAbstractCommentItem* pCommentItem = CAbstractNodeGraphViewModelItem::Cast<CAbstractCommentItem>(GetAbstractItem());
	if (pCommentItem)
	{
		QString text = pCommentItem->GetEditorData().GetText();

		m_pText->SetText(text);
		m_pText->update();

		QPointF extents = m_pStyle->GetMinimalExtents();

		qreal height = m_pText->GetTextHeight();
		height = max(height, extents.y());

		setMinimumHeight(height);
		setMaximumHeight(height);
	}
}

}
