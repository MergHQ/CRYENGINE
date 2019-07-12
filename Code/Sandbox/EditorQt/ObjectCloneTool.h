// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "LevelEditor/Tools/EditTool.h"

class CBaseObject;
class CSelectionGroup;
class CViewport;

/*!
 *	CObjectCloneTool, When created duplicate current selection, and manages cloned selection.
 *
 */

class CObjectCloneTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CObjectCloneTool)

	CObjectCloneTool();

	//////////////////////////////////////////////////////////////////////////
	// Overrides from CEditTool
	virtual string GetDisplayName() const override { return "Clone Object"; }
	bool           MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags);

	virtual void   Display(SDisplayContext& dc);
	virtual bool   OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
	virtual bool   OnKeyUp(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) { return false; }
	//////////////////////////////////////////////////////////////////////////

	void Accept();
	void Abort();

	void OnSelectionChanged();

protected:
	virtual ~CObjectCloneTool();
	// Delete itself.
	void DeleteThis() { delete this; }

private:
	void CloneSelection();
	void SetConstrPlane(CViewport* view, CPoint point);

	const CSelectionGroup* m_pSelection;
	Vec3                   m_origin;
	bool                   m_bSetConstrPlane;
	Vec3                   m_initPosition;
	//bool m_bSetCapture;
};
