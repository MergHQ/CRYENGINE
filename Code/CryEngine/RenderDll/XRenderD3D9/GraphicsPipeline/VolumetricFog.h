// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/ComputeRenderPass.h"
#include "Common/FullscreenPass.h"

class CVolumetricFogStage : public CGraphicsPipelineStage
{
public:
	static const int32 ShadowCascadeNum = 3;

	static bool  IsEnabledInFrame();
	static int32 GetVolumeTextureDepthSize();
	static int32 GetVolumeTextureSize(int32 size, int32 scale);

public:
	CVolumetricFogStage();
	virtual ~CVolumetricFogStage();

	void Init() override;
	void Prepare(CRenderView* pRenderView) override;

	void Execute();

	// TODO: refactor after removing old graphics pipeline.
	template<class RenderPassType>
	void        BindVolumetricFogResources(RenderPassType& pass, int32 startTexSlot, int32 trilinearClampSamplerSlot);
	const Vec4& GetGlobalEnvProbeShaderParam0() const { return m_globalEnvProbeParam0; }
	const Vec4& GetGlobalEnvProbeShaderParam1() const { return m_globalEnvProbeParam1; }
	CTexture*   GetGlobalEnvProbeTex0() const;
	CTexture*   GetGlobalEnvProbeTex1() const;

private:
	static const int32 MaxFrameNum = 4;

	void      InjectParticipatingMedia(CRenderView* pRenderView, const SScopedComputeCommandList& commandList);
	void      RenderVolumetricFog(CRenderView* pRenderView, const SScopedComputeCommandList& commandList);

	uint32    GetTemporalBufferId() const;
	CTexture* GetInscatterTex() const;
	CTexture* GetPrevInscatterTex() const;
	CTexture* GetDensityTex() const;
	CTexture* GetPrevDensityTex() const;

	bool      IsVisible() const;
	bool      IsTexturesValid() const;
	void      ResetFrame();
	void      UpdateFrame();
	void      RenderDownscaledShadowmap(CRenderView* pRenderView);
	void      PrepareLightList(CRenderView* pRenderView);
	void      BuildLightListGrid(const SScopedComputeCommandList& commandList);
	void      RenderDownscaledDepth(const SScopedComputeCommandList& commandList);
	void      PrepareFogVolumeList(CRenderView* pRenderView);
	void      InjectFogDensity(const SScopedComputeCommandList& commandList);
	bool      ReplaceShadowMapWithStaticShadowMap(CShadowUtils::SShadowCascades& shadowCascades, uint32 shadowCascadeSlot, CRenderView* pRenderView) const;
	void      InjectInscatteringLight(CRenderView* pRenderView, const SScopedComputeCommandList& commandList);
	void      BlurDensityVolume(const SScopedComputeCommandList& commandList);
	void      BlurInscatterVolume(const SScopedComputeCommandList& commandList);
	void      TemporalReprojection(const SScopedComputeCommandList& commandList);
	void      RaymarchVolumetricFog(const SScopedComputeCommandList& commandList);

private:
	CFullscreenPass    m_passDownscaleShadowmap[ShadowCascadeNum];
	CFullscreenPass    m_passDownscaleShadowmap2[ShadowCascadeNum];
	CComputeRenderPass m_passBuildLightListGrid;
	CComputeRenderPass m_passDownscaleDepthHorizontal;
	CComputeRenderPass m_passDownscaleDepthVertical;
	CComputeRenderPass m_passInjectFogDensity;
	CComputeRenderPass m_passInjectInscattering;
	CComputeRenderPass m_passBlurDensityHorizontal[2];
	CComputeRenderPass m_passBlurDensityVertical[2];
	CComputeRenderPass m_passBlurInscatteringHorizontal[2];
	CComputeRenderPass m_passBlurInscatteringVertical[2];
	CComputeRenderPass m_passTemporalReprojection[2];
	CComputeRenderPass m_passRaymarch[2];

	CGpuBuffer         m_lightCullInfoBuf;
	CGpuBuffer         m_LightShadeInfoBuf;
	CGpuBuffer         m_lightGridBuf;
	CGpuBuffer         m_lightCountBuf;
	CGpuBuffer         m_fogVolumeCullInfoBuf;
	CGpuBuffer         m_fogVolumeInjectInfoBuf;

	CTexture*          m_pVolFogBufDensityColor;
	CTexture*          m_pVolFogBufDensity;
	CTexture*          m_pVolFogBufEmissive;
	CTexture*          m_pInscatteringVolume;
	CTexture*          m_pFogInscatteringVolume[2];
	CTexture*          m_pFogDensityVolume[2];
	CTexture*          m_pMaxDepth;
	CTexture*          m_pMaxDepthTemp;
	CTexture*          m_pNoiseTexture;

	CTexture*          m_pDownscaledShadow[ShadowCascadeNum];
	CTexture*          m_pDownscaledShadowTemp;
	SDepthTexture      m_downscaledShadowDepthTargets[ShadowCascadeNum];
	SDepthTexture      m_downscaledShadowTempDepthTargets[ShadowCascadeNum];

	CTexture*          m_globalEnvProveTex0;
	CTexture*          m_globalEnvProveTex1;
	Vec4               m_globalEnvProbeParam0;
	Vec4               m_globalEnvProbeParam1;

	Matrix44A          m_viewProj[MaxFrameNum];
	int32              m_cleared;
	uint32             m_numTileLights;
	uint32             m_numFogVolumes;
	int32              m_frameID;
	int32              m_tick;
	int32              m_resourceFrameID;
	int32              m_samplerTrilinearClamp;
};
