#pragma once
#include "hold.h"
#include "AbnormalsMod.h"
#include "AbnormalsModLocalData.h"

class AbnormalsModGroupRestoreObj : public RestoreObj
{
public:
	AbnormalsMod *abnormalsMod;
	AbnormalsModLocalData *abnormalsModLocalData;
	
	int group;
	MaxSDK::Array<BitArray> faces;
	bool hasOldData;

	AbnormalsModGroupRestoreObj(AbnormalsMod *m, AbnormalsModLocalData *d, int group);
	void Restore(int isUndo);
	void Redo();
	int Size() { return sizeof(AbnormalsModLocalData); }
	void EndHold() { abnormalsModLocalData->held = FALSE; }
	TSTR Description() { return TSTR(_T("AbnormalsModGroupRestoreObj")); }
};