// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ObjectCloneTool.h"
#include "IEditorImpl.h"
#include "IUndoManager.h"
#include "IObjectManager.h"

#include <Objects/BaseObject.h>
#include <Objects/SelectionGroup.h>
#include <Preferences/SnappingPreferences.h>
#include <Viewport.h>

IMPLEMENT_DYNCREATE(CObjectCloneTool, CEditTool)

CObjectCloneTool::CObjectCloneTool()
{
	m_bSetConstrPlane = true;
	//m_bSetCapture = false;

	GetIEditorImpl()->GetIUndoManager()->Begin();
	m_pSelection = 0;
	if (!GetIEditorImpl()->GetSelection()->IsEmpty())
	{
		CWaitCursor wait;
		CloneSelection();
		m_pSelection = GetIEditorImpl()->GetSelection();
		GetIEditorImpl()->GetIUndoManager()->Suspend();
	}
	GetIEditor()->GetObjectManager()->signalSelectionChanged.Connect(this, &CObjectCloneTool::OnSelectionChanged);
}

void CObjectCloneTool::OnSelectionChanged()
{
	GetIEditor()->GetObjectManager()->signalSelectionChanged.DisconnectObject(this);
	Abort();
}

CObjectCloneTool::~CObjectCloneTool()
{
	GetIEditor()->GetObjectManager()->signalSelectionChanged.DisconnectObject(this);

	IUndoManager* pUndoManager = GetIEditorImpl()->GetIUndoManager();
	if (pUndoManager->IsUndoSuspended())
	{
		pUndoManager->Resume();
	}

	if (pUndoManager->IsUndoRecording())
	{
		pUndoManager->Accept("Clone");
	}
}

void CObjectCloneTool::CloneSelection()
{
	const CSelectionGroup* currSelection = GetIEditorImpl()->GetSelection();
	currSelection->FilterParents();

	std::vector<CBaseObject*> objects;
	std::vector<CBaseObject*> newObjects;
	objects.reserve(currSelection->GetFilteredCount());
	newObjects.reserve(currSelection->GetFilteredCount());

	auto pObjectManager = GetIEditor()->GetObjectManager();

	// Clone every object.
	for (int i = 0; i < currSelection->GetFilteredCount(); i++)
	{
		objects.push_back(currSelection->GetFilteredObject(i));
	}
	pObjectManager->CloneObjects(objects, newObjects);

	pObjectManager->SelectObjects(newObjects);
}

void CObjectCloneTool::SetConstrPlane(CViewport* view, CPoint point)
{
	Matrix34 originTM;
	originTM.SetIdentity();
	const CSelectionGroup* pSelection = GetIEditorImpl()->GetSelection();
	if (!pSelection->GetManipulatorMatrix(originTM))
		originTM.SetIdentity();

	m_initPosition = pSelection->GetCenter();

	view->SetConstructionMatrix(originTM);
}

void CObjectCloneTool::Display(SDisplayContext& dc)
{
	//dc.SetColor( 1,1,0,1 );
	//dc.DrawBall( gP1,1.1f );
	//dc.DrawBall( gP2,1.1f );
}

bool CObjectCloneTool::MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	if (m_pSelection)
	{
		// Set construction plane origin to selection origin.
		if (m_bSetConstrPlane)
		{
			SetConstrPlane(view, point);
			GetIEditorImpl()->GetSelection()->FilterParents();
			GetIEditorImpl()->GetSelection()->SaveFilteredTransform();
			m_bSetConstrPlane = false;
		}

		if (event == eMouseLDown)
		{
			// Accept group.
			Accept();
			GetIEditorImpl()->GetSelection()->FinishChanges();
			return true;
		}
		if (event == eMouseMove)
		{
			// Move selection.
			const CSelectionGroup* selection = GetIEditorImpl()->GetSelection();
			if (!selection->IsEmpty())
			{
				Vec3 p1 = m_initPosition;
				Vec3 p2 = view->MapViewToCP(point);
				if (p2.IsZero())
					return true;

				Vec3 offset = view->GetCPVector(p1, p2);

				offset = view->SnapToGrid(offset);

				int selectionFlags = CSelectionGroup::eMS_None;
				if (gSnappingPreferences.IsSnapToTerrainEnabled())
					selectionFlags = CSelectionGroup::eMS_FollowTerrain;

				if (gSnappingPreferences.IsSnapToGeometryEnabled())
					selectionFlags |= CSelectionGroup::eMS_FollowGeometry;

				if (gSnappingPreferences.IsSnapToNormalEnabled())
					selectionFlags |= CSelectionGroup::eMS_SnapToNormal;

				GetIEditorImpl()->GetSelection()->Move(offset, selectionFlags, point, true);
			}
		}
		if (event == eMouseWheel)
		{
			const CSelectionGroup* selection = GetIEditorImpl()->GetSelection();
			if (!selection->IsEmpty())
			{
				double angle = 1;

				if (gSnappingPreferences.angleSnappingEnabled())
					angle = gSnappingPreferences.angleSnap();

				for (int i = 0; i < selection->GetCount(); ++i)
				{
					CBaseObject* pObj = selection->GetFilteredObject(i);
					Quat rot = pObj->GetRotation();
					rot.SetRotationXYZ(Ang3(0, 0, rot.GetRotZ() + DEG2RAD(point.x > 0 ? angle * (-1) : angle)));
					pObj->SetRotation(rot);
				}
				GetIEditorImpl()->GetIUndoManager()->Accept("Rotate Selection");
			}
		}
	}
	return true;
}

void CObjectCloneTool::Abort()
{
	GetIEditorImpl()->GetIUndoManager()->Resume();
	if (GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
		GetIEditorImpl()->GetIUndoManager()->Cancel();

	CEditTool::Abort();
}

void CObjectCloneTool::Accept()
{
	GetIEditorImpl()->GetIUndoManager()->Resume();
	if (GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
		GetIEditorImpl()->GetIUndoManager()->Accept("Clone");

	GetIEditorImpl()->GetLevelEditorSharedState()->SetEditTool(nullptr);
}

bool CObjectCloneTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
	if (nChar == Qt::Key_Escape)
	{
		Abort();
	}
	return false;
}
