#pragma once

#include "FlowPaint.h"
#include "FlowPaintModData.h"

class PointRestore : public RestoreObj
{
public:
	PaintDefromModData *pmd;

	Tab<Point3> uPointList;
	Tab<Point3> rPointList;
	BOOL update;
	FlowPaint *mod;

	PointRestore(FlowPaint *mod, PaintDefromModData *c)
	{
		this->mod = mod;
		pmd = c;
		uPointList = pmd->offsetList;
		update = FALSE;
	}
	void Restore(int isUndo)
	{
		if (isUndo)
		{
			rPointList = pmd->offsetList;
		}

		pmd->offsetList = uPointList;
		if (update)
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	}
	void Redo()
	{
		pmd->offsetList = rPointList;

		if (update)
			mod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	}
	void EndHold()
	{
		mod->ClearAFlag(A_HELD);
		update = TRUE;
	}
	TSTR Description() { return TSTR(GetString(IDS_PW_PAINT)); }
};
