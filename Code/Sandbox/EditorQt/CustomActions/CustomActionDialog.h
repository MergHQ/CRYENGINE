// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// Dialog shown for custom action property
//////////////////////////////////////////////////////////////////////////
class CCustomActionDialog : public CDialog
{
	DECLARE_DYNAMIC(CCustomActionDialog)

private:
	CString  m_customAction;
	CListBox m_wndActionList;

public:
	CCustomActionDialog(CWnd* pParent = NULL);
	virtual ~CCustomActionDialog();

	void    SetCustomAction(const CString& customAction) { m_customAction = customAction; }
	CString GetCustomAction()                            { return m_customAction; }

	// Dialog Data
	enum { IDD = IDD_AIANCHORS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	bool         OpenViewForCustomAction();

	DECLARE_MESSAGE_MAP()

	afx_msg void OnLbnDblClk();
	afx_msg void OnLbnSelchangeAction();
	afx_msg void OnNewBtn();
	afx_msg void OnEditBtn();
	afx_msg void OnRefreshBtn();
public:
	virtual BOOL OnInitDialog();
};
