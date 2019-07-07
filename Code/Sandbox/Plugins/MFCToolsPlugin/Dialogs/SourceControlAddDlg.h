// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"

#define IDD_SOURCECONTROL_ADD                                                                74
#define ID_ADD_DEFAULT                                                                       33909        
#define IDC_FILENAME_SCADD                                                                   2226
#define IDC_EDIT1                                                                            1738

class MFC_TOOLS_PLUGIN_API CSourceControlAddDlg : public CDialog
{
	DECLARE_MESSAGE_MAP()

public:
	enum { IDD = IDD_SOURCECONTROL_ADD };

	enum ESCDialogResult
	{
		ADD_AND_SUBMIT,
		ADD_ONLY,
		NO_OP,
	};

	CSourceControlAddDlg(const string& sFilename, CWnd* pParent = NULL);

	// Return the state of the dialog
	ESCDialogResult GetResult() const { return m_result; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedAddDefault();
	virtual BOOL OnInitDialog();

public:

	CString m_sDesc;
	CString m_sFilename;

private:
	ESCDialogResult m_result;
};
