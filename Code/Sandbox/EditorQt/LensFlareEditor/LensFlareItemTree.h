// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "LensFlareElement.h"
#include "ILensFlareListener.h"

class CLensFlareItem;

class CLensFlareItemTree : public CXTTreeCtrl
{
	DECLARE_MESSAGE_MAP()

public:
	void UpdateLensFlareItem(HTREEITEM hSelectedItem, const CPoint& screenPos);
	void UpdateDraggingFromOtherWindow();
	bool SelectItemByCursorPos(bool* pOutChanged = NULL);
	void Drop(XmlNodeRef xmlNode, const CPoint& screenPos);

private:
	void         AssignLensFlareToLightEntity(XmlNodeRef xmlNode, const CPoint& screenPos) const;

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);

};
