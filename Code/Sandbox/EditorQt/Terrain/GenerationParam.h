// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct SNoiseParams;

class CGenerationParam : public CDialog
{
public:
	CGenerationParam(CWnd* pParent = NULL);

	void LoadParam(SNoiseParams* pParam);
	void FillParam(SNoiseParams* pParam);

	enum { IDD = IDD_GENERATION };
	int m_sldPasses;
	int m_sldFrequency;
	int m_sldFrequencyStep;
	int m_sldFade;
	int m_sldCover;
	int m_sldRandomBase;
	int m_sldSharpness;
	int m_sldBlur;

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	void         UpdateStaticNum();

	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()
};
