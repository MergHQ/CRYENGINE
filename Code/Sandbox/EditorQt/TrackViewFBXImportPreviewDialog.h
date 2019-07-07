// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CTrackViewFBXImportPreviewDialog : public CDialog
{
	DECLARE_DYNAMIC(CTrackViewFBXImportPreviewDialog)

public:
	CTrackViewFBXImportPreviewDialog();
	virtual ~CTrackViewFBXImportPreviewDialog(){}

	void AddTreeItem(const CString& objectName);
	bool IsObjectSelected(const CString& objectName) { return m_fBXItemNames[objectName]; }

private:

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void         OnClickTree(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()

private:
	typedef std::map<CString, bool> TItemsMap;
	CTreeCtrl m_tree;
	TItemsMap m_fBXItemNames;
};
