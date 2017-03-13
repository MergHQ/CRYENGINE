// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/NumberCtrl.h"

// NumberDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNumberDlg dialog

class PLUGIN_API CNumberDlg : public CDialog
{
	// Construction
public:
	CNumberDlg(CWnd* pParent = NULL, float defValue = 0, const char* title = NULL);   // standard constructor

	float        GetValue();
	void         SetRange(float min, float max);
	void         SetInteger(bool bEnable);

	CNumberCtrl& GetNumControl() { return m_num; }

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumberDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	float m_value;
	bool  m_bInteger;

	// Generated message map functions
	//{{AFX_MSG(CNumberDlg)
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CNumberCtrl m_num;
	CString m_title;
};


