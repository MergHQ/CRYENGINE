// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/ClampedSplitterWnd.h"

class CFragmentSplitter : public CClampedSplitterWnd
{
	DECLARE_DYNAMIC(CFragmentSplitter)

public:
	CFragmentSplitter() {}
	virtual ~CFragmentSplitter() {}

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnSetFocus(CWnd* pOldWnd);
};
