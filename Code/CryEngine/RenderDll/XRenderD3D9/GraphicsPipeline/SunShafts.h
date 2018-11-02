// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/FullscreenPass.h"
#include "Common/UtilityPasses.h"

class CSunShaftsStage : public CGraphicsPipelineStage
{
public:
	bool      IsStageActive(EShaderRenderingFlags flags) const final
	{
		return CRenderer::CV_r_sunshafts && CRenderer::CV_r_PostProcess;
	}

	void      Init() final;
	void      Execute();

	CTexture* GetFinalOutputRT();
	void      GetCompositionParams(Vec4& params0, Vec4& params1);

private:
	CTexture* GetTempOutputRT();
	int       GetDownscaledTargetsIndex();

private:
	CFullscreenPass m_passShaftsMask;
	CFullscreenPass m_passShaftsGen0;
	CFullscreenPass m_passShaftsGen1;
};
