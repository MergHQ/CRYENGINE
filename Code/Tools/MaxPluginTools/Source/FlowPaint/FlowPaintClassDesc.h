#pragma once

#include "FlowPaint.h"

class FlowPaintTestClassDesc :public ClassDesc2
{
public:
	static FlowPaintTestClassDesc* GetInstance();

	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new FlowPaint(); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return PAINTDEFORMTEST_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("FlowPaint"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle

};