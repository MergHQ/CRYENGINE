// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CSelectEntityClsDialog : public CXTResizeDialog
{
	DECLARE_DYNAMIC(CSelectEntityClsDialog)

public:
	CSelectEntityClsDialog(CWnd* pParent = nullptr);

	string GetEntityClass() { return m_entityClass; }

	// Dialog Data
	enum { IDD = IDD_SELECT_ENTITY_CLASS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnTvnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
	BOOL         OnInitDialog();

	void         ReloadEntities();

	DECLARE_MESSAGE_MAP()

	CTreeCtrl m_tree;
	CImageList                  m_imageList;
	string                      m_entityClass;
	std::map<HTREEITEM, string> m_itemsMap;
};
