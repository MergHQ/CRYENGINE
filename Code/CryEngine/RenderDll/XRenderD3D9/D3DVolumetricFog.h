// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*=============================================================================

   Revision history:
   Created October 2014 by Daisuke Shimizu

   =============================================================================*/

/* A short note on MultiGPU:
   Without knowing for sure if textures are copied between GPUs, the only 'correct' solution would be to have 2 * NUM_GPUs copies of rendertargets
   This would use too much memory though, so we keep one copy and swap it once every NUM_GPUs frames.This will work correctly if copy between GPUs
   is disabled. In case copies are performed though, every second frame the re-projection matrix will be off by a frame.
 */
#ifndef _D3DVOLUMETRICFOG_H_
#define _D3DVOLUMETRICFOG_H_

class CVolumetricFog
{
public:
	CVolumetricFog();
	~CVolumetricFog();

	void        CreateResources();
	void        DestroyResources(bool destroyResolutionIndependentResources);
	void        ClearAll();

	void        PrepareLightList(CRenderView* pRenderView, const RenderLightsArray* envProbes, const RenderLightsArray* ambientLights, const RenderLightsArray* defLights, uint32 firstShadowLight, uint32 curShadowPoolLight);

	void        RenderVolumetricsToVolume(void (* RenderFunc)());
	void        RenderVolumetricFog(CRenderView* pRenderView);

	void        ClearVolumeStencil();
	void        RenderClipVolumeToVolumeStencil(int nClipAreaReservedStencilBit);

	float       GetDepthIndex(float linearDepth) const;
	bool        IsEnableInFrame() const;
	const Vec4& GetGlobalEnvProbeShaderParam0() const { return m_globalEnvProbeParam0; }
	const Vec4& GetGlobalEnvProbeShaderParam1() const { return m_globalEnvProbeParam1; }
	CTexture*   GetGlobalEnvProbeTex0() const         { return m_globalEnvProveTex0; }
	CTexture*   GetGlobalEnvProbeTex1() const         { return m_globalEnvProveTex1; }

private:
	static const int32 MaxFrameNum = 4;

	void      UpdateFrame();
	bool      IsViable() const;
	void      InjectInscatteringLight(CRenderView* pRenderView);
	void      BuildLightListGrid();
	void      RaymarchVolumetricFog();
	void      BlurInscatterVolume();
	void      BlurDensityVolume();
	void      ReprojectVolume();
	CTexture* GetInscatterTex() const;
	CTexture* GetPrevInscatterTex() const;
	CTexture* GetDensityTex() const;
	CTexture* GetPrevDensityTex() const;
	void      RenderDownscaledDepth();
	void      PrepareFogVolumeList();
	void      InjectFogDensity();
	void      RenderDownscaledShadowmap();
	void      SetupStaticShadowMap(CRenderView* pRenderView, int nSlot) const;

private:
	Matrix44A              m_viewProj[MaxFrameNum];
	CTexture*              m_volFogBufDensityColor;
	CTexture*              m_volFogBufDensity;
	CTexture*              m_volFogBufEmissive;
	CTexture*              m_InscatteringVolume;
	CTexture*              m_fogInscatteringVolume[2];
	CTexture*              m_MaxDepth;
	CTexture*              m_MaxDepthTemp;
	D3DDepthSurface**      m_ClipVolumeDSVArray;
	CTexture*              m_fogDensityVolume[2];
	CTexture*              m_downscaledShadow[3];
	CTexture*              m_downscaledShadowTemp;
	CTexture*              m_globalEnvProveTex0;
	CTexture*              m_globalEnvProveTex1;

	int                    m_nTexStateTriLinear;
	int                    m_nTexStateCompare;
	int                    m_nTexStatePoint;
	int32                  m_Cleared;
	int32                  m_Destroyed;
	uint32                 m_numTileLights;
	uint32                 m_numFogVolumes;
	int32                  m_frameID;
	int32                  m_tick;
	int32                  m_ReverseDepthMode;
	float                  m_raymarchDistance;
	Vec4                   m_globalEnvProbeParam0;
	Vec4                   m_globalEnvProbeParam1;

	CGpuBuffer             m_lightCullInfoBuf;
	CGpuBuffer             m_LightShadeInfoBuf;
	CGpuBuffer             m_lightGridBuf;
	CGpuBuffer             m_lightCountBuf;
	CGpuBuffer             m_fogVolumeCullInfoBuf;
	CGpuBuffer             m_fogVolumeInjectInfoBuf;
};

#endif
