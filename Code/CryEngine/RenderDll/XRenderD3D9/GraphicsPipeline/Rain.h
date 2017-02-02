// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/UtilityPasses.h"
#include "Common/FullscreenPass.h"
#include "Common/PrimitiveRenderPass.h"

class CRainStage : public CGraphicsPipelineStage
{
public:
	CRainStage();
	virtual ~CRainStage();

	virtual void Init() override;
	virtual void Prepare(CRenderView* pRenderView) override;

	void         ExecuteRainPreprocess();
	void         ExecuteDeferredRainGBuffer();
	void         Execute();

private:
	static const int32  m_slices = 12;
	static const int32  m_maxRainLayers = 3;
	static const uint32 RainRippleTexCount = 24;

private:
	void ExecuteRainOcclusionGen(CRenderView* pRenderView);

private:
	CPrimitiveRenderPass                           m_passRainOcclusionGen;
	CStretchRectPass                               m_passCopyGBufferNormal;
	CStretchRectPass                               m_passCopyGBufferSpecular;
	CStretchRectPass                               m_passCopyGBufferDiffuse;
	CFullscreenPass                                m_passDeferredRainGBuffer;
	CFullscreenPass                                m_passRainOcclusionAccumulation;
	CGaussianBlurPass                              m_passRainOcclusionBlur;
	CPrimitiveRenderPass                           m_passRain;

	std::vector<std::unique_ptr<CRenderPrimitive>> m_rainOccluderPrimitives;

	CRenderPrimitive                               m_rainPrimitives[m_maxRainLayers];
	buffer_handle_t                                m_rainVertexBuffer = ~0u;

	CTexture* m_pSurfaceFlowTex = nullptr;
	CTexture* m_pRainSpatterTex = nullptr;
	CTexture* m_pPuddleMaskTex = nullptr;
	CTexture* m_pHighFreqNoiseTex = nullptr;
	CTexture* m_pRainfallTex = nullptr;
	CTexture* m_pRainfallNormalTex = nullptr;

	std::array<CTexture*, RainRippleTexCount> m_pRainRippleTex;
	uint32      m_rainRippleTexIndex = 0;

	SRainParams m_RainVolParams;

};
