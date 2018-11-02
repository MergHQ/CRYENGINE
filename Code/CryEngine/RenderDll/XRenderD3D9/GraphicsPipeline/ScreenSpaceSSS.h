// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/FullscreenPass.h"

// Screen Space Subsurface Scattering
class CScreenSpaceSSSStage : public CGraphicsPipelineStage
{
public:
	bool IsStageActive(EShaderRenderingFlags flags) const final
	{
		return CRendererCVars::CV_r_DeferredShadingSSS > 0;
	}

	void Execute(CTexture* pIrradianceTex);

private:
	CFullscreenPass m_passH;
	CFullscreenPass m_passV;
};
