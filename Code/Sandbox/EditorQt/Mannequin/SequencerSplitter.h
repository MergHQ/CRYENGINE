// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/ClampedSplitterWnd.h"

// CSequencerSplitter

class CSequencerSplitter : public CClampedSplitterWnd
{
	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CSequencerSplitter)

	virtual CWnd * GetActivePane(int* pRow = NULL, int* pCol = NULL)
	{
		return GetFocus();
	}

	void SetPane(int row, int col, CWnd* pWnd, SIZE sizeInit);
	// Override this for flat look.
	void OnDrawSplitter(CDC* pDC, ESplitType nType, const CRect& rectArg);

public:
	CSequencerSplitter();
};
