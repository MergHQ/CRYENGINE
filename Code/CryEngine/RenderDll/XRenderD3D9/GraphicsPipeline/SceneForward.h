// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"

class CSceneForwardStage : public CGraphicsPipelineStage
{
public:
	virtual void Init() override;
	virtual void Prepare(CRenderView* pRenderView) override;

	void         Execute_Opaque();
	void         Execute_TransparentBelowWater();
	void         Execute_TransparentAboveWater();
};
