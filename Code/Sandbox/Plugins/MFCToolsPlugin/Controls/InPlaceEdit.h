// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CInPlaceEdit : public CXTPEdit
{
public:
	typedef Functor0 OnChange;

	CInPlaceEdit(const CString& srtInitText, OnChange onchange);
	virtual ~CInPlaceEdit();

	void SetText(const CString& strText);

	void EnableUpdateOnKillFocus(bool bEnable) { m_bUpdateOnKillFocus = bEnable; }
	void SetUpdateOnEnChange(bool bEnable)     { m_bUpdateOnEnChange = bEnable; }

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInPlaceEdit)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnEnChange();

	DECLARE_MESSAGE_MAP()

protected:
	bool     m_bUpdateOnKillFocus;
	bool     m_bUpdateOnEnChange;
	CString  m_strInitText;
	OnChange m_onChange;
};
