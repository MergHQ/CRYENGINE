// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CloneTool.h"

#include "DesignerEditor.h"

#include <Objects/PrefabObject.h>
#include <Prefabs/PrefabItem.h>
#include <Prefabs/PrefabManager.h>
#include <SurfaceInfoPicker.h>
#include <ViewManager.h>

#include <Serialization/Decorators/EditorActionButton.h>

namespace Designer
{
SERIALIZATION_ENUM_BEGIN(EPlacementType, "PlacementType")
SERIALIZATION_ENUM(ePlacementType_Divide, "ePlacementType_Divide", "Divide")
SERIALIZATION_ENUM(ePlacementType_Multiply, "ePlacementType_Multiply", "Multiply")
SERIALIZATION_ENUM_END()

void CloneParameter::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::Range(m_NumberOfClones, 1, 64), "CloneCount", "Clone Count");
	if (m_PlacementType != ePlacementType_None)
		ar(m_PlacementType, "PlacementType", "Placement Type");
}

CloneTool::CloneTool(EDesignerTool tool) :
	BaseTool(tool),
	m_bSuspendedUndo(false)
{
	if (Tool() == eDesigner_CircleClone)
		m_CloneParameter.m_PlacementType = ePlacementType_None;
	else
		m_CloneParameter.m_PlacementType = ePlacementType_Divide;
}

const char* CloneTool::ToolClass() const
{
	return Serialization::getEnumDescription<EDesignerTool>().label(eDesigner_CircleClone);
}

void CloneTool::Enter()
{
	__super::Enter();

	m_vStartPos = Vec3(0, 0, 0);
	m_pSelectedClone = GetBaseObject();

	AABB boundbox;
	m_pSelectedClone->GetBoundBox(boundbox);

	m_vStartPos = GetCenterBottom(boundbox);
	m_vPickedPos = m_vStartPos;

	m_fRadius = 0.1f;

	if (!m_bSuspendedUndo)
	{
		GetIEditor()->GetIUndoManager()->Suspend();
		m_bSuspendedUndo = true;
	}

	// TODO: exit cleanly instead of falling back to other tools
	if (GetModel()->IsEmpty(eShelf_Base))
		GetDesigner()->SwitchTool(eDesigner_Invalid);

}

void CloneTool::Leave()
{
	__super::Leave();

	if (m_bSuspendedUndo)
	{
		GetIEditor()->GetIUndoManager()->Resume();
		m_bSuspendedUndo = false;
	}

	//If the selected object is still here (aka we have confirmed an array clone or just aborted) show it again
	if (m_pSelectedClone)
	{
		m_pSelectedClone->SetVisible(true);
	}

	ClearClonesList();
}

void CloneTool::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::ActionButton([this]
		{
			Confirm();
		}), "Confirm", "Confirm");
	m_CloneParameter.Serialize(ar);
}

bool CloneTool::OnLButtonDown(CViewport* view, UINT nFlags, CPoint point)
{
	UpdateCloneList();
	m_vPickedPos = m_vStartPos;
	m_Plane = BrushPlane(m_vStartPos, m_vStartPos + Vec3(0, 1, 0), m_vStartPos + Vec3(1, 0, 0));

	//We don't want to show the selected object (the one in the center of the circle) while doing the actual cloning, as it will be removed on confirm
	if (Tool() == eDesigner_CircleClone)
	{
		m_pSelectedClone->SetVisible(false);
	}

	return true;
}

bool CloneTool::OnMouseMove(CViewport* view, UINT nFlags, CPoint point)
{
	if (nFlags & MK_LBUTTON)
	{
		if (Tool() == eDesigner_ArrayClone)
		{
			CSurfaceInfoPicker picker;
			SRayHitInfo hitInfo;
			int nCloneCount = m_Clones.size();
			DESIGNER_ASSERT(nCloneCount > 0);
			if (nCloneCount > 0)
			{
				CSurfaceInfoPicker::CExcludedObjects excludedObjList;
				excludedObjList.Add(m_pSelectedClone);
				for (int i = 0; i < nCloneCount; ++i)
					excludedObjList.Add(m_Clones[i]);

				if (picker.Pick(point, hitInfo, &excludedObjList))
				{
					m_vPickedPos = hitInfo.vHitPos;
					UpdateClonePositions();
				}
			}
		}
		else if (Tool() == eDesigner_CircleClone)
		{
			Vec3 vWorldRaySrc;
			Vec3 vWorldRayDir;
			GetIEditor()->GetActiveView()->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);
			BrushVec3 vOut;
			if (m_Plane.HitTest(BrushRay(vWorldRaySrc, vWorldRayDir), NULL, &vOut))
			{
				m_vPickedPos = ToVec3(vOut);
				UpdateClonePositions();
			}
		}
	}
	return true;
}

void CloneTool::Display(struct SDisplayContext& dc)
{
	dc.SetColor(ColorB(100, 200, 100, 255));

	Matrix34 tm = dc.GetMatrix();
	dc.PopMatrix();

	if (Tool() == eDesigner_CircleClone)
	{
		dc.DrawCircle(m_vStartPos, m_fRadius);
		dc.DrawLine(m_vStartPos, Vec3(m_vPickedPos.x, m_vPickedPos.y, m_vStartPos.z));
	}
	else
	{
		dc.DrawLine(m_vStartPos, m_vPickedPos);
	}

	dc.PushMatrix(tm);
}

void CloneTool::SetObjectPivot(CBaseObject* pObj, const Vec3& pos)
{
	AABB localBB;
	pObj->GetLocalBounds(localBB);
	Vec3 vLocalCenterBottom = GetCenterBottom(localBB);
	pObj->SetPos(pos - vLocalCenterBottom);
}

Vec3 CloneTool::GetCenterBottom(const AABB& aabb) const
{
	Vec3 vCenterBottom;
	vCenterBottom.x = (aabb.min.x + aabb.max.x) * 0.5f;
	vCenterBottom.y = (aabb.min.y + aabb.max.y) * 0.5f;
	vCenterBottom.z = aabb.min.z;
	return vCenterBottom;
}

void CloneTool::OnChangeParameter(bool continuous)
{
	UpdateCloneList();
	UpdateClonePositions();
}

void CloneTool::UpdateCloneList()
{
	if (!m_pSelectedClone || m_Clones.size() == m_CloneParameter.m_NumberOfClones)
		return;

	ClearClonesList();
	m_Clones.reserve(m_CloneParameter.m_NumberOfClones);

	for (int i = 0; i < m_CloneParameter.m_NumberOfClones; ++i)
	{
		CBaseObject* pClone = GetIEditor()->GetObjectManager()->CloneObject(m_pSelectedClone);
		m_Clones.push_back(pClone);
	}

	GetIEditor()->GetViewManager()->UpdateViews();
}

void CloneTool::UpdateClonePositions()
{
	if (Tool() == eDesigner_ArrayClone)
		UpdateClonePositionsAlongLine();
	else
		UpdateClonePositionsAlongCircle();
}

void CloneTool::UpdateClonePositionsAlongLine()
{
	int nCloneCount = m_Clones.size();
	DESIGNER_ASSERT(nCloneCount > 0);
	if (nCloneCount <= 0)
		return;
	Vec3 vDelta = m_CloneParameter.m_PlacementType == ePlacementType_Divide ? (m_vPickedPos - m_vStartPos) / nCloneCount : (m_vPickedPos - m_vStartPos);
	for (int i = 1; i <= nCloneCount; ++i)
		SetObjectPivot(m_Clones[i - 1], m_vStartPos + vDelta * i);
	SetObjectPivot(m_pSelectedClone, m_vStartPos);
}

void CloneTool::UpdateClonePositionsAlongCircle()
{
	int nCloneCount = m_Clones.size();
	DESIGNER_ASSERT(nCloneCount > 0);
	if (nCloneCount <= 0)
		return;
	m_fRadius = (Vec2(m_vStartPos) - Vec2(m_vPickedPos)).GetLength();
	if (m_fRadius < 0.1f)
		m_fRadius = 0.1f;

	BrushVec2 vCenterOnPlane = m_Plane.W2P(ToBrushVec3(m_vStartPos));
	BrushVec2 vPickedPosOnPlane = m_Plane.W2P(ToBrushVec3(m_vPickedPos));
	float fAngle = ComputeAnglePointedByPos(vCenterOnPlane, vPickedPosOnPlane);

	int nCloneCountInPanel = m_CloneParameter.m_NumberOfClones;
	std::vector<BrushVec2> vertices2D;
	MakeSectorOfCircle(m_fRadius, m_Plane.W2P(ToBrushVec3(m_vStartPos)), fAngle, PI2, nCloneCountInPanel + 1, vertices2D);

	for (int i = 0; i < nCloneCountInPanel; ++i)
	{
		SetObjectPivot(m_Clones[i], ToVec3(m_Plane.P2W(vertices2D[i])));
	}
}

void CloneTool::Confirm()
{
	if (m_bSuspendedUndo)
	{
		GetIEditor()->GetIUndoManager()->Resume();
		m_bSuspendedUndo = false;
	}
	FinishCloning();
}

void CloneTool::FinishCloning()
{
	if (!m_pSelectedClone || m_Clones.empty())
	{
		return;
	}

	CPrefabManager* pPrefabManager = GetIEditor()->GetPrefabManager();
	if (!pPrefabManager)
	{
		return;
	}

	CUndo undo("Clone Designer Object");

	CSelectionGroup selectionGroup;
	selectionGroup.AddObject(m_pSelectedClone);

	// store the clones locally. Once we create a prefab from selection group, the designer object
	// is destroyed and the session ends, deleting this designer tool itself, so we need to keep a copy of
	// the objects on the stack.
	std::vector<CBaseObject*> localClones = m_Clones;

	CPrefabItem* pPrefabItem = pPrefabManager->MakeFromSelection(&selectionGroup);
	if (!pPrefabItem)
	{
		return;
	}

	const CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
	if (pSelection && !pSelection->IsEmpty())
	{
		CBaseObject* pSelected = pSelection->GetObject(0);
		if (pSelected->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			CPrefabObject* pPrefabObject = ((CPrefabObject*)pSelected);
			pPrefabObject->Open();
		}
	}

	for (int i = 0; i < localClones.size(); ++i)
	{
		CPrefabObject* pPrefabObject = (CPrefabObject*)GetIEditor()->GetObjectManager()->NewObject(PREFAB_OBJECT_CLASS_NAME, nullptr, pPrefabItem->GetGUID().ToString().c_str());
		pPrefabObject->SetPrefab(pPrefabItem, true);
		pPrefabObject->SetWorldPos(localClones[i]->GetWorldPos());
		pPrefabObject->Open();
	}

	if (Tool() == eDesigner_CircleClone)
	{
		//The circle tool removes the prefab object created from the m_pSelected as the circle (no m_pSelected in the middle of the circle)
		GetIEditor()->GetObjectManager()->DeleteObject(m_pSelectedClone->GetPrefab());
		m_pSelectedClone = nullptr;
	}

	DesignerSession::GetInstance()->SetBaseObject(nullptr);
}

void CloneTool::ClearClonesList()
{

	DesignerSession::GetInstance()->SetBaseObject(m_pSelectedClone);
	GetIEditor()->GetObjectManager()->DeleteObjects(m_Clones);
	m_Clones.clear();
}

}

REGISTER_DESIGNER_TOOL_WITH_PROPERTYTREE_PANEL_AND_COMMAND(eDesigner_CircleClone, eToolGroup_Modify, "Circle Clone", CloneTool,
                                                           circleclone, "runs Circle Clone Tool", "designer.circleclone")
REGISTER_DESIGNER_TOOL_WITH_PROPERTYTREE_PANEL_AND_COMMAND(eDesigner_ArrayClone, eToolGroup_Modify, "Array Clone", ArrayCloneTool,
                                                           arrayclone, "runs Array Clone Tool", "designer.arrayclone")
