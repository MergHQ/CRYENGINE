// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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

	void DrawTrack(int item, CDC* dc, const CRect& rcItem);
	void DrawKeys(const CSequencerTrack* track, CDC* dc, const CRect& rc, const Range& timeRange, EDSRenderFlags renderFlags);
	void DrawNodeItem(const CSequencerNode* pAnimNode, CDC* dc, const CRect& rcItem);

	// Overrides from CSequencerKeys.
	int  FirstKeyFromPoint(CPoint point, bool exact = false) const;
	int  LastKeyFromPoint(CPoint point, bool exact = false) const;
	void SelectKeys(const CRect& rc);

	int  NumKeysFromPoint(CPoint point) const;
};
