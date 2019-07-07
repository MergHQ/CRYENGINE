// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"

class MFC_TOOLS_PLUGIN_API CSplitterCtrl : public CSplitterWnd
{
public:
	DECLARE_DYNAMIC(CSplitterCtrl)

	CSplitterCtrl();
	~CSplitterCtrl();

	virtual CWnd* GetActivePane(int* pRow = NULL, int* pCol = NULL);
	//! Assign any Window to splitter window pane.
	void          SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit);
	// Override this for flat look.
	void          OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);
	void          SetNoBorders();
	void          SetTrackable(bool bTrackable);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	// override CSplitterWnd special command routing to frame
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	bool m_bTrackable;
};
