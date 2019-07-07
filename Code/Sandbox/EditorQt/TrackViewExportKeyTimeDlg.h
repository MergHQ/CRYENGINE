// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CTrackViewExportKeyTimeDlg : public CDialog
{
	DECLARE_DYNAMIC(CTrackViewExportKeyTimeDlg)

public:
	CTrackViewExportKeyTimeDlg();
	virtual ~CTrackViewExportKeyTimeDlg(){}

	bool IsAnimationExportChecked() { return m_bAnimationTimeExport; }
	bool IsSoundExportChecked()     { return m_bSoundTimeExport; }

private:

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void         SetChecks();

	DECLARE_MESSAGE_MAP()

private:
	CButton m_animationTimeExport;
	CButton m_soundTimeExport;
	bool    m_bAnimationTimeExport;
	bool    m_bSoundTimeExport;

};
