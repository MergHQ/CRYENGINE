// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "HyperGraph/HyperGraphNode.h" // includes "DisplayList.h" and <GdiPlus.h>

struct IHyperNodePainter
{
	virtual ~IHyperNodePainter() {}
	virtual void Paint(CHyperNode* pNode, CDisplayList* pList) = 0;
};
