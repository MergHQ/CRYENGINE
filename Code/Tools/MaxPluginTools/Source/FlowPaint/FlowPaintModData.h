#pragma once

class PaintDefromModData : public LocalModData
{
public:
	PaintDefromModData() {}

	//holds our offsets delta from the original 
	Tab<Point3> offsetList;

	LocalModData *Clone()
	{
		PaintDefromModData *pmd = new PaintDefromModData();
		pmd->offsetList = offsetList;
		return pmd;
	}
};