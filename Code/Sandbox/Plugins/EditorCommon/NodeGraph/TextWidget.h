// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

#include <CrySandbox/CrySignal.h>
#include <QTimer>
#include <QColor>
#include <QLineEdit>
#include <QGraphicsWidget>

namespace CryGraphEditor {

class CNodeWidget;
class CNodeGraphView;
class CTextWidgetStyle;
class CAbstractNodeItem;
class CNodeGraphViewGraphicsWidget;
class CAbstractNodeGraphViewModelItem;

class CTextWidget : public QGraphicsWidget
{
	Q_OBJECT
	friend class CTextEdit;

public:
	CTextWidget(CNodeGraphViewGraphicsWidget& viewWidget, QGraphicsWidget& parentWidget);
	virtual ~CTextWidget();

	void               TriggerEdit();

	void               SetText(const QString& text);
	QString            GetText() const { return m_text; }

	void               SetTextColor(QColor textColor) { m_textColor = textColor; }
	void               SetSelectionStyle(bool isSelected);

	 void              SetMultiline(bool multiline) { m_multiline = multiline; }
	 bool              GetMultiline() const { return m_multiline; }

	 qreal             GetTextWidth() const { return m_textWidth; }
	 qreal             GetTextHeight() const { return m_textHeight; }

public:	
	CCrySignal<void(const QString&)> SignalTextChanged;

protected:
	virtual void       mousePressEvent(QGraphicsSceneMouseEvent* pEvent) override;
	virtual void       paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;

private:
	CAbstractNodeGraphViewModelItem& m_item;
	const CTextWidgetStyle&          m_style;
	CNodeGraphViewGraphicsWidget&    m_viewWidget;

	QGraphicsWidget&                 m_parentWidget;

	QString                          m_text;
	QString                          m_textToDisplay;
	QColor                           m_textColor;
	QTimer                           m_clickTimer;
	qreal                            m_textWidth;
	qreal                            m_textHeight;
		
	bool                             m_multiline : 1;
	bool                             m_useSelectionStyle : 1;
};

}
