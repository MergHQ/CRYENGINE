// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FacialEdContext.h"
#include "Dialogs/ToolbarDialog.h"

class CImage2;

class CFacialVideoFrameDialog : public CToolbarDialog
{
	DECLARE_DYNAMIC(CFacialVideoFrameDialog)
public:

	CFacialVideoFrameDialog();
	~CFacialVideoFrameDialog();

	void         SetContext(CFacialEdContext* pContext);
	void         SetResolution(int width, int height, int bpp);
	void*        GetBits();
	int          GetPitch();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK()     {}
	virtual void OnCancel() {}
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnPaint();
	CBitmap           m_offscreenBitmap;

	CFacialEdContext* m_pContext;
	HACCEL            m_hAccelerators;
	//ATL::CImage m_image;
	CImage2*          m_image;
	//CStatic m_static;
};
