// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NodeWidget.h"
#include "GroupWidget.h"
#include "CommentWidget.h"

#include "NodeGraphView.h"
#include "HeaderWidget.h"
#include "GroupWidgetStyle.h"
#include "AbstractNodeGraphViewModel.h"
#include "NodeGraphViewGraphicsWidget.h"

#include <QPainter>
#include <QBrush>
#include <QPointF>

#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

#define GROUP_PADDING 24
#define HOVER_PADDING 7

namespace CryGraphEditor {

struct SGroupItemsLinkTracker
{
	SGroupItemsLinkTracker(CAbstractGroupItem* pGroup) : m_pGroup(pGroup)
	{
		if (m_pGroup)
		{
			m_cache = m_pGroup->GetItems();
			for (auto item : m_cache)
			{
				auto pItem = m_pGroup->GetViewModel().GetAbstractModelItemById(QVariant::fromValue(item.first));
				if (pItem)
					m_pGroup->UnlinkItem(*pItem);
			}
		}
	}

	virtual ~SGroupItemsLinkTracker()
	{		
		if (m_pGroup)
		{
			for (auto item : m_cache)
			{
				auto pItem = m_pGroup->GetViewModel().GetAbstractModelItemById(QVariant::fromValue(item.first));
				if (pItem)
					m_pGroup->LinkItem(*pItem);
			}

			m_cache.clear();
		}
	}

private:
	CGroupItems         m_cache;
	CAbstractGroupItem* m_pGroup;
};

CGroupWidget::CPendingWidget::CPendingWidget(const CGroupWidget& host)
	: m_host(host)
{
	setZValue(m_host.GetDefaultZValue() - 0.01f);
}

CGroupWidget::CPendingWidget::~CPendingWidget()
{
}

void CGroupWidget::CPendingWidget::PushPendingSelection(GraphViewWidgetSet& selection)
{
	m_pending.swap(selection);

	prepareGeometryChange();
	updateGeometry();
}

void CGroupWidget::CPendingWidget::ClearPendingSelection()
{
	m_pending.clear();

	prepareGeometryChange();
	updateGeometry();
}

void CGroupWidget::CPendingWidget::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
	static QPointF PaddingVec = QPointF(HOVER_PADDING, HOVER_PADDING);

	if (isVisible() && m_pending.size())
	{
		const CGroupWidgetStyle& style = static_cast<const CGroupWidgetStyle&>(m_host.GetStyle());
		QRectF drawRect = QRectF(boundingRect().topLeft() + PaddingVec, boundingRect().bottomRight() - PaddingVec);

		pPainter->save();
		pPainter->setBrush(style.GetPendingBackgroundColor());
		
		pPainter->setPen(QPen(QBrush(style.GetPendingBorderColor()), style.GetPendingBorderWidth(), Qt::DashLine));
		pPainter->drawRect(drawRect);

		pPainter->setBrush(Qt::NoBrush);
		pPainter->setPen(QColor(255, 0, 0));

		pPainter->restore();
	}
}

QRectF CGroupWidget::CPendingWidget::boundingRect() const
{
	const QRectF geometryRect = geometry();
	return QRectF(0, 0, geometryRect.width(), geometryRect.height());
}

void CGroupWidget::CPendingWidget::updateGeometry()
{
	QRectF aabb;

	QPointF aa = m_host.geometry().topLeft();
	QPointF bb = m_host.geometry().bottomRight();

	if (m_pending.size())
	{	
		for (auto item : m_pending)
		{
			QPointF itemAA = item->geometry().topLeft();
			QPointF itemBB = item->geometry().bottomRight();
	
			aa.setX(std::min(aa.x(), itemAA.x() - GROUP_PADDING));
			aa.setY(std::min(aa.y(), itemAA.y() - GROUP_PADDING));
	
			bb.setX(std::max(bb.x(), itemBB.x() + GROUP_PADDING));
			bb.setY(std::max(bb.y(), itemBB.y() + GROUP_PADDING));
		}
	
		aabb = QRectF(aa, bb);
	}
	
	setGeometry(aabb);
}

CGroupWidget::CGroupWidget(CNodeGraphView& view)
	: CNodeGraphViewGraphicsWidget(view)
	, m_pItem(nullptr)
	, m_role(Role_SelectionBox)	
	, m_policy(ResizePolicy_None)
	, m_pending(*this)
	, m_pHeader(nullptr)
{
	SetDefaultZValue(0.8);	
}

CGroupWidget::CGroupWidget(CAbstractGroupItem& item, CNodeGraphView& view)
	: CNodeGraphViewGraphicsWidget(view)
	, m_pItem(&item)
	, m_role(Role_GrouppingBox)
	, m_policy(ResizePolicy_None)
	, m_pending(*this)
	, m_pHeader(nullptr)
	, m_pLayout(nullptr)
{
	m_pStyle = GetView().GetStyle().GetGroupWidgetStyle(item.GetStyleId());
	CRY_ASSERT(m_pStyle);
	if (!m_pStyle)
		m_pStyle = GetView().GetStyle().GetGroupWidgetStyle("Group");

	m_pLayout = new QGraphicsLinearLayout();
	m_pLayout->setOrientation(Qt::Vertical);
	m_pLayout->setSpacing(0.0f);	
	m_pLayout->setContentsMargins(HOVER_PADDING, HOVER_PADDING, HOVER_PADDING, m_pStyle->GetBorderWidth());

	m_pHeader = new CHeaderWidget(*this, false);
	m_pLayout->addItem(m_pHeader);

	m_pItem->SignalAABBChanged.Connect(this, &CGroupWidget::OnAABBChanged);
	m_pItem->SignalInvalidated.Connect(this, &CGroupWidget::OnItemInvalidated);
	m_pItem->SignalPositionChanged.Connect(this, &CGroupWidget::OnGroupPositionChanged);
	m_pItem->SignalItemsChanged.Connect(this, &CGroupWidget::OnGroupItemsChanged);

	setLayout(m_pLayout);
	setAcceptHoverEvents(true);

	SetDefaultZValue(0.8);
	SetColor(m_pStyle->GetBackgroundColor());
}

CGroupWidget::~CGroupWidget()
{
	if (m_pItem)
	{
		m_pItem->SignalItemsChanged.DisconnectObject(&GetView());
		m_pItem->SignalAABBChanged.DisconnectObject(this);
		m_pItem->SignalInvalidated.DisconnectObject(this);
		m_pItem->SignalPositionChanged.DisconnectObject(this);
	}
}

void CGroupWidget::DeleteLater()
{
	CNodeGraphViewGraphicsWidget::DeleteLater();

	QGraphicsLinearLayout* pLayout = static_cast<QGraphicsLinearLayout*>(layout());
	if (pLayout)
	{
		pLayout->removeItem(m_pHeader);
		delete m_pHeader;
	}
}

void CGroupWidget::OnItemInvalidated()
{	
	CNodeGraphViewGraphicsWidget::OnItemInvalidated();
	m_pHeader->SetName(GetAbstractItem()->GetName());
}

const CNodeGraphViewStyleItem& CGroupWidget::GetStyle() const
{ 
	return *m_pStyle; 
}

QPointF CGroupWidget::GetAA() const
{
	if (m_pItem)
	{
		Vec2 aa = m_pItem->GetEditorData().GetAA();
		return QPointF(aa.x, aa.y);
	}
	
	return m_aa; 
}

void CGroupWidget::SetAA(const QPointF& aa)
{
	if (m_pItem)
	{
		m_pItem->GetEditorData().SetAA(Vec2(aa.x(), aa.y()));
	}
	//TODO: remove this;
	else 
	{
		m_aa = aa;
	}
	//~TODO:

	prepareGeometryChange();
	updateGeometry();
}

QPointF CGroupWidget::GetBB() const
{
	if (m_pItem)
	{
		Vec2 bb = m_pItem->GetEditorData().GetBB();
		return QPointF(bb.x, bb.y);
	}

	return m_bb; //TODO: remove this
}

void CGroupWidget::SetBB(const QPointF& bb)
{
	if (m_pItem)
	{
		m_pItem->GetEditorData().SetBB(Vec2(bb.x(), bb.y()));
	}
	//TODO: remove this
	else 
	{
		m_bb = bb;
	}
	//TODO:

	prepareGeometryChange();
	updateGeometry();
}

void CGroupWidget::SetAABB(const QRectF& aabb)
{
	SetAA(aabb.topLeft());
	SetBB(aabb.bottomRight());

	prepareGeometryChange();
	updateGeometry();
}

void CGroupWidget::UpdateAABB()
{
	SGroupItemsLinkTracker tracker(CAbstractNodeGraphViewModelItem::Cast<CAbstractGroupItem>(GetAbstractItem()));
	UpdateAABB([](QPointF&, QPointF&) {});
}

void CGroupWidget::CalcResize(const QPointF& pos)
{
	SGroupItemsLinkTracker tracker(CAbstractNodeGraphViewModelItem::Cast<CAbstractGroupItem>(GetAbstractItem()));

	const auto policy = m_policy;
	UpdateAABB([pos, policy](QPointF& aa, QPointF& bb)
	{
		switch (policy)
		{
			case ResizePolicy_Top:
			{
				aa.setY(pos.y());
				break;
			}
			case ResizePolicy_Bottom:
			{
				bb.setY(pos.y());
				break;
			}
			case ResizePolicy_Right:
			{
				bb.setX(pos.x());
				break;
			}
			case ResizePolicy_Left:
			{
				aa.setX(pos.x());
				break;
			}
			case ResizePolicy_TopLeft:
			{
				aa = pos;
				break;
			}
			case ResizePolicy_BottomRight:
			{
				bb = pos;
				break;
			}
			case ResizePolicy_TopRight:
			{
				aa.setY(pos.y());
				bb.setX(pos.x());
				break;
			}
			case ResizePolicy_BottomLeft:
			{
				aa.setX(pos.x());
				bb.setY(pos.y());
				break;
			}
		}
	});
}

CGroupWidget::EResizePolicy CGroupWidget::CalcPolicy(const QPointF& pos)
{
	static const int32   Eps = 3;
	static const QPointF Pad = QPointF(HOVER_PADDING, HOVER_PADDING);

	QRectF rect = QRectF(geometry().topLeft() + Pad, geometry().bottomRight() - Pad);
	QPoint aa   = rect.topLeft().toPoint();
	QPoint bb   = rect.bottomRight().toPoint();
	QPoint p    = pos.toPoint();

	int32 x0 = max<int32>(p.x() - aa.x(), Eps*sgn(aa.x() - p.x()));
	int32 x1 = max<int32>(bb.x() - p.x(), Eps*sgn(p.x() - bb.x()));

	int32 y0 = max<int32>(p.y() - aa.y(), Eps*sgn(aa.y() - p.y()));
	int32 y1 = max<int32>(bb.y() - p.y(), Eps*sgn(p.y() - bb.y()));

	m_policy = static_cast<CGroupWidget::EResizePolicy>(
		((x0 < Eps) && (y0 < Eps))                 * CGroupWidget::ResizePolicy_TopLeft     +
		((x1 < Eps) && (y1 < Eps))                 * CGroupWidget::ResizePolicy_BottomRight +
		((x1 < Eps) && (y0 < Eps))                 * CGroupWidget::ResizePolicy_TopRight    +
		((x0 < Eps) && (y1 < Eps))                 * CGroupWidget::ResizePolicy_BottomLeft  +
		((y1 < Eps) && (x0 >= Eps) && (x1 >= Eps)) * CGroupWidget::ResizePolicy_Bottom      +
		((y0 < Eps) && (x0 >= Eps) && (x1 >= Eps)) * CGroupWidget::ResizePolicy_Top         +
		((x1 < Eps) && (y0 >= Eps) && (y1 >= Eps)) * CGroupWidget::ResizePolicy_Right       +
		((x0 < Eps) && (y0 >= Eps) && (y1 >= Eps)) * CGroupWidget::ResizePolicy_Left
	);

	return m_policy;
}

void CGroupWidget::PushPendingSelection(GraphViewWidgetSet& selection)
{
	CRY_ASSERT(m_role == Role_GrouppingBox);
	m_pending.PushPendingSelection(selection);
}

void CGroupWidget::ClearPendingSelection()
{
	CRY_ASSERT(m_role == Role_GrouppingBox);
	m_pending.ClearPendingSelection();
}

void CGroupWidget::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
	static QPointF PaddingVec = QPointF(HOVER_PADDING, HOVER_PADDING);
	if (isVisible())
	{
		QRectF groupRect;
		switch (m_role)
		{
			case Role_SelectionBox:
			{
				groupRect = boundingRect();
				break;
			}
			case Role_GrouppingBox:
			{
				groupRect = QRectF(boundingRect().topLeft() + PaddingVec, boundingRect().bottomRight() - PaddingVec);
				break;
			}
		}

		pPainter->save();

		pPainter->setBrush(m_color);
		pPainter->setPen(m_color);

		pPainter->drawRect(groupRect);

		pPainter->setBrush(Qt::NoBrush);
		pPainter->setPen(QColor(255, 0, 0));

		pPainter->restore();
	}
}

QRectF CGroupWidget::boundingRect() const
{
	const QRectF geometryRect = geometry();
	return QRectF(0, 0, geometryRect.width(), geometryRect.height());
}

void CGroupWidget::updateGeometry()
{
	QPointF AA = GetAA();
	QPointF BB = GetBB();

	const qreal x = std::min(AA.x(), BB.x());
	const qreal y = std::min(AA.y(), BB.y());

	const qreal w = std::abs(AA.x() - BB.x());
	const qreal h = std::abs(AA.y() - BB.y());

	const QPointF topLeft = QPointF(x, y);
	const QRectF g = QRectF(topLeft, topLeft + QPointF(w, h));

	setGeometry(g);
}

void CGroupWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::HoverEnter,
		Qt::MouseButton::NoButton, Qt::MouseButton::NoButton, pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);

}

void CGroupWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::HoverMove,
		Qt::MouseButton::NoButton, Qt::MouseButton::NoButton, pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);
}

void CGroupWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::HoverLeave,
		Qt::MouseButton::NoButton, Qt::MouseButton::NoButton, pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);
}

void CGroupWidget::mousePressEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::ButtonPress,
		pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);	
}

void CGroupWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::ButtonRelease,
		pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);
}

void CGroupWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent)
{
	SMouseInputEventArgs mouseArgs = SMouseInputEventArgs(
		EMouseEventReason::ButtonPressAndMove,
		pEvent->button(), pEvent->buttons(), pEvent->modifiers(),
		pEvent->pos(), pEvent->scenePos(), pEvent->screenPos(),
		pEvent->lastPos(), pEvent->lastScenePos(), pEvent->lastScreenPos());

	SGroupMouseEventArgs args(mouseArgs);
	GetView().OnGroupMouseEvent(this, args);

	UpdateSelectionBox(pEvent->scenePos());
}

QSizeF CGroupWidget::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
	if (m_role == Role_GrouppingBox)
	{
		switch (which)
		{
			case Qt::MinimumSize:
			{
				return QSizeF(m_pStyle->GetMinimalExtents().x(), m_pStyle->GetMinimalExtents().y());
			}
			case Qt::MaximumSize:
			{
				return QSizeF(1024.0f*m_pStyle->GetMinimalExtents().x(), 768.0f*m_pStyle->GetMinimalExtents().y());
			}
		}
	}

	return QGraphicsWidget::sizeHint(which, constraint);
}

void CGroupWidget::OnAABBChanged()
{
	prepareGeometryChange();
	updateGeometry();
}

void CGroupWidget::OnGroupItemsChanged()
{
	CAbstractGroupItem* pGroup = CAbstractNodeGraphViewModelItem::Cast<CAbstractGroupItem>(GetAbstractItem());
	CRY_ASSERT_MESSAGE(pGroup, "Something has gone wrong. This method couldn't be invoked in his group item is null.");

	if (!pGroup)
	{
		return;
	}

	// resolve group items
	std::vector<CAbstractNodeGraphViewModelItem*> items;
	for (auto it : pGroup->GetItems())
	{
		QVariant id = QVariant::fromValue(it.first);
		CNodeGraphViewModel& viewModel = pGroup->GetViewModel();
		CAbstractNodeGraphViewModelItem* pItem = viewModel.GetAbstractModelItemById(id);

		if (pItem)
		{
			items.push_back(pItem);
		}
	}

	// resolve group items widgets
	std::vector<CNodeGraphViewGraphicsWidget*> widgets;
	for (auto pItem : items)
	{
		CNodeGraphViewGraphicsWidget* pWidget = nullptr;
		switch (pItem->GetType())
		{
			case eItemType_Node: pWidget = GetView().GetNodeWidget(*pItem->Cast<CAbstractNodeItem>()); break;
			case eItemType_Comment: pWidget = GetView().GetCommentWidget(*pItem->Cast<CAbstractCommentItem>()); break;
		}

		if (pWidget)
		{
			widgets.push_back(pWidget);
		}
	}

	// finally link to group
	for (auto pWidget : widgets)
	{
		GetView().LinkItemToGroup(*this, *pWidget);
	}
}

void CGroupWidget::OnGroupPositionChanged()
{
	if (CAbstractNodeGraphViewModelItem* pAbstractItem = GetAbstractItem())
	{
		CRY_ASSERT(pAbstractItem->GetAcceptsMoves());
		setPos(pAbstractItem->GetPosition());

		SetAABB(QRectF(geometry().topLeft(), geometry().bottomRight()));
	}
}

void CGroupWidget::UpdateAABB(std::function<void(QPointF& aa, QPointF& bb)> userFunc)
{
	QPointF aa = geometry().topLeft();
	QPointF bb = geometry().bottomRight();

	userFunc(aa, bb);

	for (auto pItem : m_items)
	{
		QPointF itemAA = pItem->geometry().topLeft();
		QPointF itemBB = pItem->geometry().bottomRight();

		aa.setX(std::min(aa.x(), itemAA.x() - GROUP_PADDING));
		aa.setY(std::min(aa.y(), itemAA.y() - GROUP_PADDING - 20));

		bb.setX(std::max(bb.x(), itemBB.x() + GROUP_PADDING));
		bb.setY(std::max(bb.y(), itemBB.y() + GROUP_PADDING));
	}

	SetAABB(QRectF(aa,bb));
	GetAbstractItem()->SetPosition(aa);
}

void CGroupWidget::UpdateContainingSceneItems()
{
	m_items.clear();

	QList<QGraphicsItem*> items = GetView().scene()->items(geometry());
	for (QGraphicsItem* pItem : items)
	{
		CNodeGraphViewGraphicsWidget* pGraphWidget = qgraphicsitem_cast<CNodeGraphViewGraphicsWidget*>(pItem);
		const int32 widgetType = pGraphWidget ? pGraphWidget->GetType() : eGraphViewWidgetType_Unset;

		switch (widgetType)
		{
			case eGraphViewWidgetType_NodeWidget:
			case eGraphViewWidgetType_CommentWidget:
			case eGraphViewWidgetType_ConnectionWidget:
			{
				m_items.insert(pGraphWidget);
			}
			break;
		}
	}
}

void CGroupWidget::UpdateSelectionBox(const QPointF& pos)
{
	if (m_role == Role_SelectionBox)
	{
		m_bb = pos;

		prepareGeometryChange();
		updateGeometry();
	}
}

}
