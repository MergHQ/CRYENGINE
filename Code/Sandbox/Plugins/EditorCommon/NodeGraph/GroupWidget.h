// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "NodeGraphViewGraphicsWidget.h"
#include "EditorCommonAPI.h"

#include "HeaderWidget.h"
#include "AbstractGroupItem.h"

#include <QString>
#include <QColor>
#include <QRectF>
#include <QPointF>

class QGraphicsLinearLayout;

namespace CryGraphEditor {

class CNodeGraphView;

class EDITOR_COMMON_API CGroupWidget : public CNodeGraphViewGraphicsWidget
{
	Q_OBJECT

	enum ERole : int32
	{
		Role_GrouppingBox = 0,
		Role_SelectionBox,
	};

	class CPendingWidget : public QGraphicsWidget
	{
	public:
		CPendingWidget(const CGroupWidget& host);
		~CPendingWidget();

		void                      PushPendingSelection(GraphViewWidgetSet& selection);
		void                      ClearPendingSelection();

	protected:
		virtual void              paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
		virtual QRectF            boundingRect() const override;
		virtual void              updateGeometry() override;

	private:
		const CGroupWidget&       m_host;
		GraphViewWidgetSet        m_pending;
	};

public:
	friend class CNodeGraphView;
	enum : int32 { Type = eGraphViewWidgetType_GroupWidget };

	enum EResizePolicy : int32
	{
		ResizePolicy_None = 0,
		ResizePolicy_Top,
		ResizePolicy_Bottom,
		ResizePolicy_Right,
		ResizePolicy_Left,
		ResizePolicy_TopLeft,
		ResizePolicy_TopRight,
		ResizePolicy_BottomLeft,
		ResizePolicy_BottomRight
	};

public:
	CGroupWidget(CNodeGraphView& view);
	CGroupWidget(CAbstractGroupItem& item, CNodeGraphView& view);
	~CGroupWidget();

	// CNodeGraphViewGraphicsWidget
	virtual void                             DeleteLater() override;
	virtual int32                            GetType() const override { return Type; }
	virtual CAbstractNodeGraphViewModelItem* GetAbstractItem() const  { return m_pItem; }
	virtual void                             OnItemInvalidated() override;
	virtual const CNodeGraphViewStyleItem&   GetStyle() const override;
	// ~CNodeGraphViewGraphicsWidget

	const QString&            GetText() const                         { return m_text; }
	void                      SetText(const QString& text)            { m_text = text; }

	const QColor&             GetColor() const                        { return m_color; }
	void                      SetColor(const QColor& color)           { m_color = color; }

	QPointF                   GetAA() const;
	void                      SetAA(const QPointF& positon);

	QPointF                   GetBB() const;
	void                      SetBB(const QPointF& positon);

	const QRectF              GetAABB() const                         { return QRectF(GetAA(),GetBB()); }
	void                      SetAABB(const QRectF& aabb);

	const QRectF&             GetRect() const                         { return m_rect; }
	GraphViewWidgetSet&       GetItems()                              { return m_items; }

	void                      UpdateAABB();

	void                      CalcResize(const QPointF& pos);
	EResizePolicy             CalcPolicy(const QPointF& pos);

	void                      PushPendingSelection(GraphViewWidgetSet& selection);
	void                      ClearPendingSelection();

	QGraphicsWidget*          GetHeaderWidget()                       { return m_pHeader; }
	QGraphicsWidget*          GetPendingWidget()                      { return &m_pending; }

protected:
	virtual void              paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
	virtual QRectF            boundingRect() const override;
	virtual void              updateGeometry() override;

	virtual void              hoverEnterEvent(QGraphicsSceneHoverEvent* pEvent) override;
	virtual void              hoverMoveEvent(QGraphicsSceneHoverEvent* pEvent) override;
	virtual void              hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent) override;

	virtual void              mousePressEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void              mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void              mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent) override;

	virtual QSizeF            sizeHint(Qt::SizeHint which, const QSizeF & constraint = QSizeF()) const override;

protected:
	void                      OnAABBChanged();
	void                      OnGroupItemsChanged();
	void                      OnGroupPositionChanged();

private:
	void                      UpdateAABB(std::function<void(QPointF& aa, QPointF& bb)> userFunc);
	void                      UpdateContainingSceneItems();
	void                      UpdateSelectionBox(const QPointF& pos);

private:
	const CGroupWidgetStyle*  m_pStyle;
	ERole                     m_role;
	QString                   m_text;
	QColor                    m_color;
	EResizePolicy             m_policy;

	//TODO: remove this, right now this is only needed for compatibility with the selection box
	QPointF                   m_aa;
	QPointF                   m_bb;
	//~TODO:

	QRectF                    m_rect;
	CAbstractGroupItem*       m_pItem;
	GraphViewWidgetSet        m_items;
	CPendingWidget            m_pending;
	CHeaderWidget*            m_pHeader;
	QGraphicsLinearLayout*    m_pLayout;
};

}
