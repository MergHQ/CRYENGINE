// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ResizeResolutionDialog.h"

CResizeResolutionDialog::CResizeResolutionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CResizeResolutionDialog::IDD, pParent)
{
}

void CResizeResolutionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RESOLUTION, m_resolution);
}

BEGIN_MESSAGE_MAP(CResizeResolutionDialog, CDialog)
ON_CBN_SELENDOK(IDC_RESOLUTION, OnCbnSelendokResolution)
END_MESSAGE_MAP()

BOOL CResizeResolutionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_resolution.AddString(_T("64x64"));
	m_resolution.AddString(_T("128x128"));
	m_resolution.AddString(_T("256x256"));
	m_resolution.AddString(_T("512x512"));
	m_resolution.AddString(_T("1024x1024"));
	m_resolution.AddString(_T("2048x2048"));

	uint32 dwSel = m_dwInitSize / 64;

	int sel = 0;
	while (dwSel != 1 && dwSel > 0)
	{
		dwSel = dwSel >> 1;
		sel++;
	}

	m_curSel = sel;
	m_resolution.SetCurSel(sel);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

}

void CResizeResolutionDialog::OnCbnSelendokResolution()
{
	m_curSel = m_resolution.GetCurSel();
}

void CResizeResolutionDialog::SetSize(uint32 dwSize)
{
	m_dwInitSize = dwSize;
}

uint32 CResizeResolutionDialog::GetSize()
{
	int sel = m_curSel;
	if (sel >= 0)
	{
		return 64 * (1 << sel);
	}
	return m_dwInitSize;
}
