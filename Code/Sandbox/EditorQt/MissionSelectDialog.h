// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CMissionSelectDialog : public CDialog
{
public:
	CMissionSelectDialog(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CMissionSelectDialog)
	enum { IDD = IDD_MISSIONS };
	CListBox m_missions;
	CString  m_description;
	CString  m_selected;
	//}}AFX_DATA

	CString GetSelected() { return m_selected; }

	// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectMission();
	afx_msg void OnDblclkMissions();
	afx_msg void OnUpdateDescription();
	DECLARE_MESSAGE_MAP()

	std::vector<CString> m_descriptions;
};
