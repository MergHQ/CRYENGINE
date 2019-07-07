// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ICryGraphEditor.h"
#include "EditorCommonAPI.h"

#include "NodeGraphStyleItem.h"

#include <QColor>
#include <QMargins>

namespace CryGraphEditor {

class EDITOR_COMMON_API CCommentWidgetStyle : public CNodeGraphViewStyleItem
{
	Q_OBJECT;
	friend class CHeaderWidgetStyle;

public:
	Q_PROPERTY(QColor borderColor READ GetBorderColor WRITE SetBorderColor DESIGNABLE true);
	Q_PROPERTY(int32 borderWidth READ GetBorderWidth WRITE SetBorderWidth DESIGNABLE true);
	Q_PROPERTY(QColor backgroundColor READ GetBackgroundColor WRITE SetBackgroundColor DESIGNABLE true);
	Q_PROPERTY(QMargins margins READ GetMargins WRITE SetMargins DESIGNABLE true);

public:
	CCommentWidgetStyle(const char* szStyleId, CNodeGraphViewStyle& viewStyle);

	const QPointF&                  GetMinimalExtents() const                  { return m_minimalExtents; }
	void                            SetMinimalExtents(const QPointF& extents)  { m_minimalExtents = extents; }

	const QColor&                   GetBorderColor() const                     { return m_borderColor; }
	void                            SetBorderColor(const QColor& color)        { m_borderColor = color; }

	int32                           GetBorderWidth() const                     { return m_borderWidth; }
	void                            SetBorderWidth(int32 width)                { m_borderWidth = width; }

	const QColor&                   GetBackgroundColor() const                 { return m_backgroundColor; }
	void                            SetBackgroundColor(const QColor color)     { m_backgroundColor = color; }

	QMargins                        GetMargins() const                         { return m_margins; }
	void                            SetMargins(QMargins margins)               { m_margins = margins; }

private:	
	QColor                          m_borderColor;
	int32                           m_borderWidth;
	QPointF                         m_minimalExtents;
	QColor                          m_backgroundColor;
	QMargins                        m_margins;
};

}
