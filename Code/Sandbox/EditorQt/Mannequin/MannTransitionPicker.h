// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "MannequinBase.h"
#include "Controls/PropertiesPanel.h"

// fwd decl'
struct SScopeContextData;

class CMannTransitionPickerDlg : public CXTResizeDialog
{
public:
	CMannTransitionPickerDlg(FragmentID& fromFragID, FragmentID& toFragID, SFragTagState& fromFragTag, SFragTagState& toFragTag, CWnd* pParent = NULL);
	virtual ~CMannTransitionPickerDlg();

	afx_msg void OnOk();
	afx_msg void OnComboChange();

protected:
	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

private:
	void UpdateFragmentIDs();
	void PopulateComboBoxes();

	//
	FragmentID& m_IDFrom;
	FragmentID& m_IDTo;

	//
	SFragTagState& m_TagsFrom;
	SFragTagState& m_TagsTo;

	// "From" fragment ID selection
	CComboBox m_fromComboBox;

	// "To" fragment ID selection
	CComboBox m_toComboBox;
};
