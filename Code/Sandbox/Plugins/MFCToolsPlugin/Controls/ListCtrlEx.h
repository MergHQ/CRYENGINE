// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MFCToolsDefines.h"

//////////////////////////////////////////////////////////////////////////
class MFC_TOOLS_PLUGIN_API CListCtrlEx : public CXTListCtrl
{
public:
	CListCtrlEx();
	virtual ~CListCtrlEx();

	void RepositionControls();

	CWnd* GetItemControl(int nIndex, int nColumn);
	void  SetItemControl(int nIndex, int nColumn, CWnd* pWnd);
	BOOL  DeleteAllItems();

protected:
	virtual BOOL SubclassWindow(HWND hWnd);

	DECLARE_MESSAGE_MAP()
	afx_msg void    OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void    OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void    OnSize(UINT nType, int cx, int cy);
	afx_msg void    OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
	afx_msg void    OnHeaderEndTrack(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);

	typedef std::map<int, CWnd*> ControlsMap;
	ControlsMap m_controlsMap;
};

//////////////////////////////////////////////////////////////////////////
class MFC_TOOLS_PLUGIN_API CControlsListBox : public CListBox
{
public:
	CControlsListBox();
	virtual ~CControlsListBox();

	void  RepositionControls();
	CWnd* GetItemControl(int nIndex);
	void  SetItemControl(int nIndex, CWnd* pWnd);
	void  ResetContent();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void    OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void    OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void    OnSize(UINT nType, int cx, int cy);
	afx_msg void    OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);

	typedef std::map<int, CWnd*> ControlsMap;
	ControlsMap m_controlsMap;
};
