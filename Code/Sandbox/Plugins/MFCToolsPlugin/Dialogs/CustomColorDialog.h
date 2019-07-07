// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MFCToolsDefines.h"

// This class extents CMFCColorDialog for two reasons:
//  1. Enables the 'ColorChangeCallback' registration
//  2. Makes the color picking tool work across multiple monitors
class MFC_TOOLS_PLUGIN_API CCustomColorDialog : public CMFCColorDialog
{
	DECLARE_DYNAMIC(CCustomColorDialog)
public:
	typedef Functor1<COLORREF> ColorChangeCallback;

	CCustomColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0, CWnd* pParentWnd = NULL);

	void SetColorChangeCallback(ColorChangeCallback cb)
	{ m_callback = cb; }

	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
	//{{AFX_MSG(CCustomColorDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM lCount);
	afx_msg void OnColorSelect();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnColorChange(COLORREF col);

	ColorChangeCallback m_callback;
	COLORREF            m_colorPrev;
};
