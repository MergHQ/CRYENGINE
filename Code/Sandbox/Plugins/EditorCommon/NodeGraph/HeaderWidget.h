// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "TextWidget.h"

#include <QGraphicsWidget>

class QGraphicsLinearLayout;

namespace CryGraphEditor {

class CNodeWidget;
class CNodeGraphView;
class CHeaderIconWidget;
class CHeaderWidgetStyle;

class CHeaderWidget : public QGraphicsWidget
{
public:
	enum class EIconSlot
	{
		Left,
		Right,
	};

public:
	CHeaderWidget(CNodeGraphViewGraphicsWidget& viewWidget, bool bApplyIcon = true);

	QString GetName() const;
	void    SetName(QString name);
	void    EditName();

	void    AddIcon(CHeaderIconWidget* pHeaderIcon, EIconSlot slot);
	void    SetNameWidth(int32 width);
	void    SetNameColor(QColor color);

public:
	void    OnNameChanged();
	void    OnSelectionChanged(bool isSelected);
	

protected:
	virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;

private:
	const CHeaderWidgetStyle*     m_pStyle;
	CNodeGraphViewGraphicsWidget& m_viewWidget;
	CNodeGraphView&               m_view;
	QGraphicsLinearLayout*        m_pLayout;
	CTextWidget                   m_name;
};

}
