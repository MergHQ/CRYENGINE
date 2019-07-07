// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ICryGraphEditor.h"
#include "EditorCommonAPI.h"

#include <QWidget>

namespace CryGraphEditor {

class CTextWidgetStyle;
class CHeaderWidgetStyle;
class CNodeGraphViewStyle;

typedef uint32 StyleIdHash;

class EDITOR_COMMON_API CNodeGraphViewStyleItem : public QWidget
{
	Q_OBJECT;
	friend class CHeaderWidgetStyle;

public:
	CNodeGraphViewStyleItem(const char* szStyleId);

	const char*                     GetId() const                { return m_styleId.c_str(); }
	StyleIdHash                     GetIdHash() const            { return m_styleIdHash; }

	CNodeGraphViewStyle*            GetViewStyle() const;

	QWidget*                        GetParent() const            { return parentWidget(); }
	void                            SetParent(QWidget* pParent)  { setParent(pParent); }
	void                            SetParent(CNodeGraphViewStyle* pViewStyle);

	const QColor&                   GetSelectionColor() const; 
	const QColor&                   GetHighlightColor() const;

	const CTextWidgetStyle&         GetHeaderTextStyle() const   { return *m_pTextStyle; }
	CTextWidgetStyle&               GetHeaderTextStyle()         { return *m_pTextStyle; }

	const CHeaderWidgetStyle&       GetHeaderWidgetStyle() const { return *m_pHeaderStyle; }
	CHeaderWidgetStyle&             GetHeaderWidgetStyle()       { return *m_pHeaderStyle; }

	const QIcon&                    GetTypeIcon() const;

protected:
	virtual CTextWidgetStyle*       CreateTextWidgetStyle();
	virtual CHeaderWidgetStyle*     CreateHeaderWidgetStyle();

private:
	CTextWidgetStyle*               m_pTextStyle;
	CHeaderWidgetStyle*             m_pHeaderStyle;

	const string                    m_styleId;
	const StyleIdHash               m_styleIdHash;
	CNodeGraphViewStyle*            m_pViewStyle;
};

}
