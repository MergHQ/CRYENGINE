// Copyright 2001-2015 Crytek GmbH. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/PrimitiveRenderPass.h"
#include "Common/FullscreenPass.h"
//#include "Common/UtilityPasses.h"

class CClipVolumesStage : public CGraphicsPipelineStage
{
public:
	static const uint32 MaxDeferredClipVolumes = 64;
	static const uint32 VisAreasOutdoorStencilOffset = 2; // Note: 2 stencil values reserved for stencil+outdoor fragments

public:
	CClipVolumesStage();
	virtual ~CClipVolumesStage();

	void Init();
	void Prepare(CRenderView* pRenderView) final;
	void Execute();

	void GetClipVolumeShaderParams(const Vec4*& pParams, uint32& paramCount)
	{
		CRY_ASSERT(m_bClipVolumesValid);
		pParams = m_clipVolumeShaderParams;
		paramCount = m_nShaderParamCount;
	}

	bool      IsOutdoorVisible() { CRY_ASSERT(m_bClipVolumesValid); return m_bOutdoorVisible; }

	CTexture* GetClipVolumeStencilVolumeTexture() const;

private:
	void PrepareVolumetricFog();
	void ExecuteVolumetricFog();

private:
	CPrimitiveRenderPass m_stencilPass;
	CPrimitiveRenderPass m_blendValuesPass;
	CFullscreenPass      m_stencilResolvePass;

	CPrimitiveRenderPass m_volumetricStencilPass;
	CFullscreenPass      m_passWriteJitteredDepth;

	CRenderPrimitive     m_stencilPrimitives[MaxDeferredClipVolumes * 2];
	CRenderPrimitive     m_blendPrimitives[MaxDeferredClipVolumes];

	CRY_ALIGN(16) Vec4 m_clipVolumeShaderParams[MaxDeferredClipVolumes];
	uint32                     m_nShaderParamCount;

	CTexture*                  m_pBlendValuesRT;
	SDepthTexture*             m_pDepthTarget;

	CTexture*                  m_pClipVolumeStencilVolumeTex;
	SDepthTexture              m_depthTargetVolFog;
	std::vector<SDepthTexture> m_depthTargetArrayVolFog;

	int32                      m_cleared;
	float                      m_nearDepth;
	float                      m_raymarchDistance;

	bool                       m_bClipVolumesValid;
	bool                       m_bBlendPassReady;
	bool                       m_bOutdoorVisible;
};
