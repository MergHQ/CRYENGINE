// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CResizeResolutionDialog : public CDialog
{
public:
	CResizeResolutionDialog(CWnd* pParent = nullptr);

	enum { IDD = IDD_RESIZETILERESOLUTION };

	void   SetSize(uint32 dwSize);
	uint32 GetSize();

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void         OnCbnSelendokResolution();

	DECLARE_MESSAGE_MAP()

	CComboBox m_resolution;
	uint32 m_dwInitSize;
	int    m_curSel;
};