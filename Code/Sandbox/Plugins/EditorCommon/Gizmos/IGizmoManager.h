// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "ITransformManipulator.h"
#include "IEditor.h" // for EMouseEvent

class CGizmo;
struct SDisplayContext;
struct HitContext;
struct IDisplayViewport;

/** GizmoManager manages set of currently active Gizmo objects.
 */
struct EDITOR_COMMON_API IGizmoManager
{
	virtual ~IGizmoManager() {}
	virtual ITransformManipulator* AddManipulator(ITransformManipulatorOwner* pOwner) = 0;
	virtual void                   RemoveManipulator(ITransformManipulator* pManipulator) = 0;

	virtual void                   AddGizmo(CGizmo* gizmo) = 0;
	virtual void                   RemoveGizmo(CGizmo* gizmo) = 0;

	virtual int                    GetGizmoCount() const = 0;
	virtual CGizmo*                GetGizmoByIndex(int nIndex) const = 0;

	virtual CGizmo*                GetHighlightedGizmo() const = 0;

	virtual void                   Display(SDisplayContext& dc) = 0;

	virtual bool                   HandleMouseInput(IDisplayViewport*, EMouseEvent event, CPoint& point, int flags) = 0;
};
