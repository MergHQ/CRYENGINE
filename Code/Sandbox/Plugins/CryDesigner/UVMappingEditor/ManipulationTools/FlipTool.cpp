// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FlipTool.h"
#include "UVMappingEditor/UVCluster.h"
#include "UVMappingEditor/UVMappingEditor.h"
#include "UVMappingEditor/UVUndo.h"

namespace Designer {
namespace UVMapping
{
void FlipTool::Flip(const Vec2& normal)
{
	UVElementSetPtr pElementSet = GetUVEditor()->GetElementSet();

	Vec3 pivot = GetUVEditor()->GetPivotType() == ePivotType_Selection ? pElementSet->GetCenterPos() : GetUVEditor()->GetUVCursor()->GetPos();
	UVCluster uvCluster;
	uvCluster.Flip(Vec2(pivot.x, pivot.y), normal);

	GetUVEditor()->ApplyPostProcess();
}

void FlipHoriTool::Enter()
{
	if (!GetUVEditor()->GetElementSet()->IsEmpty())
	{
		CUndo undo("UVMapping Editor : Flip");
		CUndo::Record(new UVMoveUndo);
		Flip(Vec2(1.0f, 0.0f));
	}
}

void FlipVertTool::Enter()
{
	if (!GetUVEditor()->GetElementSet()->IsEmpty())
	{
		CUndo undo("UVMapping Editor : Flip");
		CUndo::Record(new UVMoveUndo);
		Flip(Vec2(0.0f, 1.0f));
	}
}
}
}

REGISTER_UVMAPPING_TOOL_AND_COMMAND(eUVMappingTool_FlipHori, eUVMappingToolGroup_Manipulation, "Flip Horizontal", FlipHoriTool,
                                    flip_hori, "runs flip hori tool", "designer.flip_hori")
REGISTER_UVMAPPING_TOOL_AND_COMMAND(eUVMappingTool_FlipVert, eUVMappingToolGroup_Manipulation, "Flip Vertical", FlipVertTool,
                                    flip_vert, "runs flip vert tool", "designer.flip_vert")
