#pragma once
#include "hold.h"
#include "bitarray.h"
#include "polymeshseldata.h"
#include "SubSelectionPolyMod.h"

class SelectRestore : public RestoreObj
{
public:
	BitArray usel, rsel;
	BitArray *sel;
	SubSelectionPolyMod *mod;
	PolyMeshSelData *d;
	int level;

	SelectRestore(SubSelectionPolyMod *m, PolyMeshSelData *d);
	SelectRestore(SubSelectionPolyMod *m, PolyMeshSelData *d, int level);
	void Restore(int isUndo);
	void Redo();
	int Size() { return 1; }
	void EndHold() { d->held = FALSE; }
	TSTR Description() { return TSTR(_T("SelectRestore")); }
};