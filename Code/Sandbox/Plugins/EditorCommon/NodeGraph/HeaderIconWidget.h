// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

#include <QGraphicsWidget>

class QGraphicsLinearLayout;

namespace CryGraphEditor {

class CNodeWidget;
class CAbstractNodeItem;
class CNodeGraphViewGraphicsWidget;
class CAbstractNodeGraphViewModelItem;

class EDITOR_COMMON_API CHeaderIconWidget : public QGraphicsWidget
{
public:
	CHeaderIconWidget(CNodeGraphViewGraphicsWidget& viewWidget);
	virtual ~CHeaderIconWidget() {}

	virtual void OnClicked()    {}
	virtual void OnMouseEnter() {}
	virtual void OnMouseLeave() {}

protected:
	void               SetDisplayIcon(const QPixmap* pPixmap) { m_pPixmap = pPixmap; update(); }
	const QPixmap*     GetDisplayIcon() const                 { return m_pPixmap; }

	CNodeGraphViewGraphicsWidget&    GetViewWidget() const    { return m_viewWidget; }
	CAbstractNodeGraphViewModelItem& GetViewItem() const;

private:
	virtual void   paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;

	virtual void   hoverEnterEvent(QGraphicsSceneHoverEvent* pEvent) override;
	virtual void   hoverLeaveEvent(QGraphicsSceneHoverEvent* pEvent) override;

	virtual void   mousePressEvent(QGraphicsSceneMouseEvent* pEvent) override {}
	virtual void   mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void   mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent) override  {}

	virtual QRectF boundingRect() const override;

	virtual QSizeF sizeHint(Qt::SizeHint which, const QSizeF& constraint) const override;

private:
	const QPixmap*                   m_pPixmap;	
	CNodeGraphViewGraphicsWidget&    m_viewWidget;
	CAbstractNodeGraphViewModelItem& m_viewItem;
};

}
