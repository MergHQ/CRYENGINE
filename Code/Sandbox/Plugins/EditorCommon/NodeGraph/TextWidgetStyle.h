// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ICryGraphEditor.h"
#include "EditorCommonAPI.h"

#include "NodeGraphStyleItem.h"
#include "NodeGraphStyleUtils.h"
#include "NodeWidgetStyle.h"

#include <QWidget>
#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QFont>

namespace CryGraphEditor {

class EDITOR_COMMON_API CTextWidgetStyle : public QWidget
{
	Q_OBJECT;

public:
	Q_PROPERTY(QFont textFont READ GetTextFont WRITE SetTextFont DESIGNABLE true);
	Q_PROPERTY(QColor textColor READ GetTextColor WRITE SetTextColor DESIGNABLE true);
	
	Q_PROPERTY(int32 height READ GetHeight WRITE SetHeight DESIGNABLE true);
	Q_PROPERTY(QMargins margins READ GetMargins WRITE SetMargins DESIGNABLE true);

public:
	CTextWidgetStyle(CNodeGraphViewStyleItem& viewItemStyle);

	const CNodeGraphViewStyleItem& GetViewItemStyle() const { return m_viewItemStyle; }

	//
	int               GetAlignment() const                  { return m_alignment; }
	void              SetAlignment(int alignment)           { m_alignment = alignment; }

	const QFont&      GetTextFont() const                   { return m_textFont; }
	void              SetTextFont(const QFont& font)        { m_textFont = font; }

	const QColor&     GetTextColor() const                  { return m_textColor; }
	void              SetTextColor(const QColor& color)     { m_textColor = color; }

	int32             GetHeight() const                     { return m_height; }
	void              SetHeight(int32 height)               { m_height = height; }

	const QMargins&   GetMargins() const                    { return m_margins; }
	void              SetMargins(const QMargins& margins)   { m_margins = margins; }

	//
	// TODO: Colors for left + right.
	const QColor&     GetHighlightColor() const             { return m_viewItemStyle.GetHighlightColor(); }
	// ~TODO

private:
	const CNodeGraphViewStyleItem& m_viewItemStyle;

	int      m_alignment;
	QFont    m_textFont;
	QColor   m_textColor;
	int32    m_height;
	QMargins m_margins;
};

}
