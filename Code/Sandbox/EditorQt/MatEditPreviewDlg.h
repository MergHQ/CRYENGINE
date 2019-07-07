// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Dialogs/ToolbarDialog.h"
#include "Controls/PreviewModelCtrl.h"
#include "IDataBaseManager.h"

class CMatEditPreviewDlg : public CToolbarDialog, public IDataBaseManagerListener
{
public:
	CMatEditPreviewDlg(const char* title = NULL, CWnd* pParent = NULL);
	~CMatEditPreviewDlg();

	enum { IDD = IDD_MATEDITPREVIEWDLG };

	virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event);

protected:
	virtual void PostNcDestroy();

	virtual BOOL OnInitDialog();
	afx_msg void OnPreviewSphere();
	afx_msg void OnPreviewPlane();
	afx_msg void OnPreviewBox();
	afx_msg void OnPreviewTeapot();
	afx_msg void OnPreviewCustom();
	afx_msg void OnUpdateAlways();
	afx_msg void OnUpdateAlwaysUpdateUI(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnKickIdle(WPARAM wParam, LPARAM);
	DECLARE_MESSAGE_MAP()

private:
	CPreviewModelCtrl m_previewCtrl;
};
