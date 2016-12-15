#pragma once
#include "GeomDecal.h"

class GeomDecalGeneralDlgProc : public ParamMap2UserDlgProc 
{
private:
	GeomDecal* geomDecal;
public:
	GeomDecalGeneralDlgProc(GeomDecal* s) { geomDecal = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class GeomDecalDumpHitDialog : public HitByNameDlgCallback 
{
public:
	GeomDecal* eo;
	GeomDecalDumpHitDialog(GeomDecal *e) { eo = e; }
	const TCHAR* dialogTitle() { return _T("Add"); }
	const TCHAR* buttonText() { return _T("Add"); }
	BOOL singleSelect() { return FALSE; }
	BOOL useProc() { return TRUE; }
	int filter(INode *node);
	void proc(INodeTab &nodeTab);
};