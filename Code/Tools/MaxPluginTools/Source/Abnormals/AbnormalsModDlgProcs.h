#pragma once
#include "AbnormalsMod.h"

class AbnormalsModChannelsDlgProc : public ParamMap2UserDlgProc 
{
private:
	AbnormalsMod* abnormalsMod;
public:
	AbnormalsModChannelsDlgProc(AbnormalsMod* s) { abnormalsMod = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};

class AbnormalsModEditGroupsDlgProc : public ParamMap2UserDlgProc
{
private:
	AbnormalsMod* abnormalsMod;
public:
	AbnormalsModEditGroupsDlgProc(AbnormalsMod* s) { abnormalsMod = s;}
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void UpdateEditGroupsDlg(HWND hWnd);
	void DeleteThis() { delete this; }
};

class AbnormalsModAutoChamferDlgProc : public ParamMap2UserDlgProc
{
private:
	AbnormalsMod* abnormalsMod;
public:
	AbnormalsModAutoChamferDlgProc(AbnormalsMod* s) { abnormalsMod = s; }
	INT_PTR DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void DeleteThis() { delete this; }
};