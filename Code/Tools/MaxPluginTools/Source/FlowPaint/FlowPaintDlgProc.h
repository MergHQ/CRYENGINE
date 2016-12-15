#include "stdafx.h"

class FlowPaint;

class PaintDeformTestDlgProc : public ParamMap2UserDlgProc
{
public:
	FlowPaint *mod;
	PaintDeformTestDlgProc(FlowPaint *m) { mod = m; }
	INT_PTR DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};