// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "NumberDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CNumberDlg dialog

CNumberDlg::CNumberDlg(CWnd* pParent /*=NULL*/, float defValue, const char* title)
	: CDialog(IDD_NUMBER, pParent)
{
	//{{AFX_DATA_INIT(CNumberDlg)
	//}}AFX_DATA_INIT
	m_value = defValue;

	if (title)
		m_title = title;
}

void CNumberDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNumberDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNumberDlg, CDialog)
//{{AFX_MSG_MAP(CNumberDlg)
ON_WM_CLOSE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumberDlg message handlers

void CNumberDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnClose();
}

float CNumberDlg::GetValue()
{
	m_value = m_num.GetValue();
	return m_value;
}

void CNumberDlg::SetRange(float min, float max)
{
	m_num.SetRange(min, max);
}

void CNumberDlg::SetInteger(bool bEnable)
{
	m_num.SetInteger(bEnable);
}

BOOL CNumberDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_num.Create(this, IDC_NUMBER);
	m_num.SetValue(m_value);

	if (!m_title.IsEmpty())
		SetWindowText(m_title);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
