#include "stdafx.h"
#include "GeomDecalDlgProcs.h"

#include <WinUser.h>

#include "mesh.h"
#include "MeshNormalSpec.h"

#include "GeomDecalClassDesc.h"
#include "GeomDecalCreateCallback.h"
#include "GeomDecalParamBlock.h"

INT_PTR GeomDecalGeneralDlgProc::DlgProc(TimeValue t, IParamMap2 * map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	geomDecal->UpdateControls(hWnd, eGeomDecalRollout::general);

	switch (msg)
	{
	case WM_COMMAND:
		int loword = LOWORD(wParam);

		switch (LOWORD(wParam))
		{
		case IDC_ADDOBJ:
		{
			geomDecal->ip->DoHitByNameDialog(new GeomDecalDumpHitDialog(geomDecal));
			break;
		}
		default:
			break;
		}
	}
	return FALSE;
}

void GeomDecalDumpHitDialog::proc(INodeTab &nodeTab)
{
	int nodeCount = nodeTab.Count();

	if (nodeCount == 0) return;

	theHold.Begin();

	for (int i = 0; i<nodeTab.Count(); i++)
	{
		eo->pblock2->Append(eGeomDecalParam::node_pool, 1, &nodeTab[i]);
	}
	theHold.Accept(_T("Add"));
	eo->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	eo->ip->RedrawViews(eo->ip->GetTime());
}

int GeomDecalDumpHitDialog::filter(INode *node)
{
	for (int i = 0; i < eo->pblock2->Count(eGeomDecalParam::node_pool); i++)
	{
		INode* otherNode = eo->pblock2->GetINode(eGeomDecalParam::node_pool, 0, i);

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