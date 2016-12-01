#pragma once
#include "SuperArray.h"

class SuperArrayGeneralDlgProc : public ParamMap2UserDlgProc 
{
private:
	SuperArray* superArray;
public:
	SuperArrayGeneralDlgProc(SuperArray* s) { superArray = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class SuperArrayOffsetDlgProc : public ParamMap2UserDlgProc
{
private:
	SuperArray* superArray;
public:
	SuperArrayOffsetDlgProc(SuperArray* s) { superArray = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class SuperArrayRotationDlgProc : public ParamMap2UserDlgProc
{
private:
	SuperArray* superArray;
public:
	SuperArrayRotationDlgProc(SuperArray* s) { superArray = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class SuperArrayScaleDlgProc : public ParamMap2UserDlgProc
{
private:
	SuperArray* superArray;
public:
	SuperArrayScaleDlgProc(SuperArray* s) { superArray = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class DumpHitDialog : public HitByNameDlgCallback 
{
public:
	SuperArray* eo;
	DumpHitDialog(SuperArray *e) { eo = e; }
	const TCHAR* dialogTitle() { return _T("Add"); }
	const TCHAR* buttonText() { return _T("Add"); }
	BOOL singleSelect() { return FALSE; }
	BOOL useProc() { return TRUE; }
	int filter(INode *node);
	void proc(INodeTab &nodeTab);
};