// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class COverwriteConfirmDialog : public CDialog
{
public:
	COverwriteConfirmDialog(CWnd* pParentWindow, const char* szMessage, const char* szCaption);

private:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg BOOL OnCommand(UINT uID);

	DECLARE_MESSAGE_MAP()

	string message;
	string caption;
	CEdit  messageEdit;
};
