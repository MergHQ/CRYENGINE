// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controls/ImageListCtrl.h"

class CLensFlareEditor;

class CLensFlareAtomicList : public CImageListCtrl
{
public:

	struct CFlareImageItem : public CImageListCtrlItem
	{
		EFlareType flareType;
	};

	CLensFlareAtomicList();
	virtual ~CLensFlareAtomicList();

	void FillAtomicItems();

protected:

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

	bool OnBeginDrag(UINT nFlags, CPoint point);
	void OnDropItem(CPoint point);
	void OnDraggingItem(CPoint point);
	bool InsertItem(const FlareInfo& flareInfo);
};
