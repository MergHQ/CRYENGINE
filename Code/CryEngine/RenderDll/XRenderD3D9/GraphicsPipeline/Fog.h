// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/FullscreenPass.h"

class CFogStage : public CGraphicsPipelineStage
{
public:
	CFogStage();
	virtual ~CFogStage();

	void Init() override;
	void Prepare(CRenderView* pRenderView) override;

	void Execute();

private:
	void ExecuteVolumetricFogShadow();
	f32  GetFogCullDistance() const;

private:
#if defined(VOLUMETRIC_FOG_SHADOWS)
	CFullscreenPass m_passVolFogShadowRaycast;
	CFullscreenPass m_passVolFogShadowHBlur;
	CFullscreenPass m_passVolFogShadowVBlur;
#endif
	CFullscreenPass m_passFog;
	CFullscreenPass m_passLightning;

	CTexture*       m_pTexInterleaveSamplePattern;

	int32           m_samplerPointClamp;
};
