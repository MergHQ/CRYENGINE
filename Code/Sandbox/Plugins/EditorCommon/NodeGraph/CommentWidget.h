// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "NodeGraphViewGraphicsWidget.h"
#include "EditorCommonAPI.h"

#include "AbstractCommentItem.h"

#include <QColor>
#include <QRectF>
#include <QPointF>
#include <QString>
#include <QGraphicsLinearLayout>

namespace CryGraphEditor {

class CTextWidget;
class CNodeGraphView;
class CAbstractCommentItem;

class EDITOR_COMMON_API CCommentWidget : public CNodeGraphViewGraphicsWidget
{
	Q_OBJECT

public:

	enum : int32 { Type = eGraphViewWidgetType_CommentWidget };

public:
	CCommentWidget(CAbstractCommentItem& item, CNodeGraphView& view);
	~CCommentWidget();

	// CNodeGraphViewGraphicsWidget
	virtual void                             DeleteLater() override;
	virtual int32                            GetType() const override      { return Type;    }
	virtual const CNodeGraphViewStyleItem&   GetStyle() const override;
	virtual CAbstractNodeGraphViewModelItem* GetAbstractItem() const override;
	// ~CNodeGraphViewGraphicsWidget

protected:
	virtual void               paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
	virtual QRectF             boundingRect() const override;

	virtual void               mousePressEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void               mouseReleaseEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void               mouseMoveEvent(QGraphicsSceneMouseEvent* pEvent) override;

	virtual void               moveEvent(QGraphicsSceneMoveEvent* pEvent) override;

protected:
	void                       OnItemTextChanged();

private:
	const CCommentWidgetStyle* m_pStyle;

	QColor                     m_color;
	CTextWidget*               m_pText;

	CAbstractCommentItem&      m_item;
	QGraphicsLinearLayout*     m_pLayout;
};

}
