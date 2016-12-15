#include "stdafx.h"
#include "SuperArrayDlgProcs.h"

#include <WinUser.h>

#include "mesh.h"
#include "MeshNormalSpec.h"

#include "SuperArrayClassDesc.h"
#include "SuperArrayCreateCallback.h"
#include "SuperArrayParamBlock.h"

INT_PTR SuperArrayGeneralDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	superArray->UpdateControls(hWnd, eSuperArrayRollout::general);

	switch (msg)
	{
	case WM_COMMAND:
		int loword = LOWORD(wParam);

		switch (LOWORD(wParam))
		{
		case IDC_ADDOBJ:
		{
			superArray->ip->DoHitByNameDialog(new DumpHitDialog(superArray));
			break;
		}
		default:
			break;
		}
	}
	return FALSE;
}

INT_PTR SuperArrayOffsetDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	superArray->UpdateControls(hWnd,eSuperArrayRollout::offset);
	return FALSE;
}

INT_PTR SuperArrayRotationDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	superArray->UpdateControls(hWnd, eSuperArrayRollout::rotation);
	return FALSE;
}

INT_PTR SuperArrayScaleDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	superArray->UpdateControls(hWnd, eSuperArrayRollout::scale);
	return FALSE;
}

void DumpHitDialog::proc(INodeTab &nodeTab)
{
	int nodeCount = nodeTab.Count();

	if (nodeCount == 0) return;

	theHold.Begin();

	for (int i = 0; i<nodeTab.Count(); i++)
	{
		eo->pblock2->Append(eSuperArrayParam::node_pool, 1, &nodeTab[i]);
	}
	theHold.Accept(_T("Add"));
	eo->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	eo->ip->RedrawViews(eo->ip->GetTime());
}

int DumpHitDialog::filter(INode *node)
{
	for (int i = 0; i < eo->pblock2->Count(eSuperArrayParam::node_pool); i++)
	{
		INode* otherNode = eo->pblock2->GetINode(eSuperArrayParam::node_pool, 0, i);

		if (node == otherNode)
			return FALSE;
	}

	ObjectState os = node->EvalWorldState(0);

	if (os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID)
		return FALSE;

	node->BeginDependencyTest();
	eo->NotifyDependents(FOREVER, 0, REFMSG_TEST_DEPENDENCY);
	if (node->EndDependencyTest())
	{
		return FALSE;
	}

	return TRUE;
}