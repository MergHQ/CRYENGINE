#include "stdafx.h"
#include "SelectRestore.h"

SelectRestore::SelectRestore(SubSelectionPolyMod *m, PolyMeshSelData *data) {
	mod = m;
	level = mod->GetCurrentSubSelectionType();
	d = data;
	d->held = TRUE;
	switch (level) {
	case SubSelectionPolyMod::SEL_OBJECT: DbgAssert(0); break;
	case SubSelectionPolyMod::SEL_VERTEX: usel = d->vertSel; break;
	case SubSelectionPolyMod::SEL_EDGE:
	case SubSelectionPolyMod::SEL_BORDER:
		usel = d->edgeSel;
		break;
	default:
		usel = d->faceSel;
		break;
	}
}

SelectRestore::SelectRestore(SubSelectionPolyMod *m, PolyMeshSelData *data, int sLevel) {
	mod = m;
	level = sLevel;
	d = data;
	d->held = TRUE;
	switch (level) {
	case SubSelectionPolyMod::SEL_OBJECT: DbgAssert(0); break;
	case SubSelectionPolyMod::SEL_VERTEX: usel = d->vertSel; break;
	case SubSelectionPolyMod::SEL_EDGE:
	case SubSelectionPolyMod::SEL_BORDER:
		usel = d->edgeSel; break;
	default:
		usel = d->faceSel; break;
	}
}

void SelectRestore::Restore(int isUndo) {
	if (isUndo) {
		switch (level) {
		case SubSelectionPolyMod::SEL_VERTEX:
			rsel = d->vertSel; break;
		case SubSelectionPolyMod::SEL_FACE:
		case SubSelectionPolyMod::SEL_ELEMENT:
			rsel = d->faceSel; break;
		case SubSelectionPolyMod::SEL_EDGE:
		case SubSelectionPolyMod::SEL_BORDER:
			rsel = d->edgeSel; break;
		}
	}
	switch (level) {
	case SubSelectionPolyMod::SEL_VERTEX:
		d->vertSel = usel; break;
	case SubSelectionPolyMod::SEL_FACE:
	case SubSelectionPolyMod::SEL_ELEMENT:
		d->faceSel = usel; break;
	case SubSelectionPolyMod::SEL_BORDER:
	case SubSelectionPolyMod::SEL_EDGE:
		d->edgeSel = usel; break;
	}
	mod->LocalDataChanged();
}

void SelectRestore::Redo() {
	switch (level) {
	case SubSelectionPolyMod::SEL_VERTEX:
		d->vertSel = rsel; break;
	case SubSelectionPolyMod::SEL_FACE:
	case SubSelectionPolyMod::SEL_ELEMENT:
		d->faceSel = rsel; break;
	case SubSelectionPolyMod::SEL_EDGE:
	case SubSelectionPolyMod::SEL_BORDER:
		d->edgeSel = rsel; break;
	}
	mod->LocalDataChanged();
}