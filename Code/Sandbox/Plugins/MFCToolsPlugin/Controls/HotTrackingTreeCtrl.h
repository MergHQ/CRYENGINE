// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "MFCToolsDefines.h"

class MFC_TOOLS_PLUGIN_API CHotTrackingTreeCtrl : public CTreeCtrl
{
public:
	CHotTrackingTreeCtrl();
	virtual ~CHotTrackingTreeCtrl(){}

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

private:
	HTREEITEM m_hHoverItem;
};
