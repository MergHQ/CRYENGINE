// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Editor/Schematyc_IQuickSearchOptions.h>

namespace Schematyc
{
	class CQuickSearchEditCtrl : public CEdit
	{
		DECLARE_MESSAGE_MAP()

	private:

		afx_msg UINT OnGetDlgCode();
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	};

	class CQuickSearchDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

		friend class CQuickSearchEditCtrl;

	public:

		CQuickSearchDlg(CWnd* pParent, CPoint pos, const IQuickSearchOptions& options, const char* szDefaultOption = nullptr, const char* szDefaultFilter = nullptr);

		virtual ~CQuickSearchDlg();

		const uint32 GetResult() const;

	private:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnFilterChanged();
		afx_msg void OnTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnViewWiki();

		void RefreshTreeCtrl(const char* szDefaultOption);
		void UpdateSelection();
		HTREEITEM FindTreeCtrlItem(HTREEITEM hRootItem, const char* name, bool recursive) const;
		HTREEITEM GetPrevTreeCtrlItem(HTREEITEM hItem);
		HTREEITEM GetNextTreeCtrlItem(HTREEITEM hItem, bool ignoreChildren);
		HTREEITEM GetLastTreeCtrlItemChild(HTREEITEM hItem);

		CPoint                     m_pos;
		const IQuickSearchOptions& m_options;
		string                     m_defaultOption;
		string                     m_defaultFilter;
		CQuickSearchEditCtrl       m_filterCtrl;
		CTreeCtrl                  m_treeCtrl;
		CStatic                    m_descriptionCtrl;
		CButton                    m_viewWikiButton;
		CButton                    m_okButton;
		uint32                     m_result;
	};
}