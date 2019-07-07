// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SequencerDopeSheetBase.h"

/** List of tracks.
 */
class CSequencerDopeSheet : public CSequencerDopeSheetBase
{
	DECLARE_DYNAMIC(CSequencerDopeSheet)
public:
	// public stuff.

	CSequencerDopeSheet();
	~CSequencerDopeSheet();

protected:
	DECLARE_MESSAGE_MAP()

	void DrawTrack(int item, CDC* dc, CRect& rcItem);
	void DrawKeys(CSequencerTrack* track, CDC* dc, CRect& rc, Range& timeRange, EDSRenderFlags renderFlags);
	void DrawNodeItem(CSequencerNode* pAnimNode, CDC* dc, CRect& rcItem);

	// Overrides from CSequencerKeys.
	int  FirstKeyFromPoint(CPoint point, bool exact = false) const;
	int  LastKeyFromPoint(CPoint point, bool exact = false) const;
	void SelectKeys(const CRect& rc);

	int  NumKeysFromPoint(CPoint point) const;
};
