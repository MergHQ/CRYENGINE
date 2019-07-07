// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CFolderTreeCtrl;
class CPreviewerPage;

class CSequenceBrowser : public CXTResizeFormView
{
	DECLARE_DYNAMIC(CSequenceBrowser)

public:
	CSequenceBrowser(CWnd* parent, CPreviewerPage& previewerPage);
	virtual ~CSequenceBrowser();

	enum { IDD = IDD_SEQUENCE_BROWSER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	afx_msg void OnTreeDoubleClicked(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOpen();

	CFolderTreeCtrl* m_pTree;
	CPreviewerPage&  m_previewerPage;

	DECLARE_MESSAGE_MAP()
};
