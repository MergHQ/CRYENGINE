// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CTerrainFormulaDlg : public CDialog
{
public:
	CTerrainFormulaDlg(CWnd* pParent = nullptr);

	enum { IDD = IDD_TERRAIN_FORMULA };
	double m_dParam1;
	double m_dParam2;
	double m_dParam3;

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()
};