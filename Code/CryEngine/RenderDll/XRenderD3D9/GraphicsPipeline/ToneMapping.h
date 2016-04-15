// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/FullscreenPass.h"

class CToneMappingStage : public CGraphicsPipelineStage
{
public:
	void Init();
	void Execute();

private:
	CFullscreenPass m_passToneMapping;

	int             m_samplerPoint;
	int             m_samplerLinear;
};
