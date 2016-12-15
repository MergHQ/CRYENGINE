#include "stdafx.h"
#include "AbnormalsModDlgProcs.h"

#include <WinUser.h>

#include "AbnormalsModClassDesc.h"
#include "AbnormalsModParamBlock.h"

INT_PTR AbnormalsModChannelsDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	abnormalsMod->UpdateControls(hWnd, eAbnormalsModRollout::channels);
	return FALSE;
}

INT_PTR AbnormalsModEditGroupsDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UpdateEditGroupsDlg(hWnd);
	switch (msg)
	{
	case WM_INITDIALOG:
		
		break;
	case WM_COMMAND:
		int loword = LOWORD(wParam);

		switch (LOWORD(wParam))
		{
		case IDC_ASSIGNGROUP_1:
		{
			abnormalsMod->AssignGroupToSelectedFaces(0, true);
			break;
		}
		case IDC_ASSIGNGROUP_2:
		{
			abnormalsMod->AssignGroupToSelectedFaces(1, true);
			break;
		}
		case IDC_ASSIGNGROUP_3:
		{
			abnormalsMod->AssignGroupToSelectedFaces(2, true);
			break;
		}
		case IDC_ASSIGNGROUP_CLEAR:
		{
			abnormalsMod->AssignGroupToSelectedFaces(3, true);
			break;
		}
		case IDC_SELECTGROUP_1:
		{
			abnormalsMod->SelectGroup(0, t);
			break;
		}
		case IDC_SELECTGROUP_2:
		{
			abnormalsMod->SelectGroup(1, t);
			break;
		}
		case IDC_SELECTGROUP_3:
		{
			abnormalsMod->SelectGroup(2, t);
			break;
		}
		case IDC_SELECTGROUP_4:
		{
			abnormalsMod->SelectGroup(3, t);
			break;
		}
		default:
			break;
		}
		break;
	}
	return FALSE;
}

INT_PTR AbnormalsModAutoChamferDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	abnormalsMod->UpdateControls(hWnd, eAbnormalsModRollout::auto_chamfer);
	return FALSE;
}

void AbnormalsModEditGroupsDlgProc::UpdateEditGroupsDlg(HWND hWnd)
{
	static HWND lasthWnd = NULL;
	// Store the last hWnd, so you're able to call this function with a null hWnd as long as it's been called once before.
	// Super hacky, but it's difficult to get the rollout HWND outside of the DlgProc call, which isn't called when sub-object level is changed.
	if (hWnd == NULL)
	{
		if (lasthWnd == NULL)
			return;
		else
			hWnd = lasthWnd;
	}
	else
		lasthWnd = hWnd;

	abnormalsMod->UpdateControls(hWnd, eAbnormalsModRollout::general);	
}