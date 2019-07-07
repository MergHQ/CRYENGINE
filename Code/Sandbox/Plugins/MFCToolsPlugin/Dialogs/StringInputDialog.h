// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// This is the header file for the general utility dialog for
// string input. The purpose of this dialog, as one might imagine, is to get
// string input for any purpose necessary.
// The recomended way to call this dialog is through DoModal() method.

#include "MFCToolsDefines.h"

class MFC_TOOLS_PLUGIN_API CStringInputDialog : public CDialog
{
public:
	CStringInputDialog();
	CStringInputDialog(CString strText, CString strTitle);
	void    DoDataExchange(CDataExchange* pDX);
	BOOL    OnInitDialog();
	void    OnOK();

	void    SetText(CString strText);
	void    SetTitle(CString strTitle);

	CString GetResultingText();

protected:
	CString m_strTitle;
	CString m_strText;
	CEdit   m_nameEdit;
};
