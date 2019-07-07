// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// This is the header file for importing background objects into Mannequin.
// The recomended way to call this dialog is through DoModal() method.

class CMannImportBackgroundDialog : public CDialog
{
public:
	CMannImportBackgroundDialog(std::vector<CString>& loadedObjects);

	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	void OnOK();

	int  GetCurrentRoot() const
	{
		return m_selectedRoot;
	}

protected:
	const std::vector<CString>& m_loadedObjects;
	CComboBox                   m_comboRoot;
	int                         m_selectedRoot;
};
