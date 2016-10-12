// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/UtilityPasses.h"
#include "Common/FullscreenPass.h"
#include "Common/PrimitiveRenderPass.h"
#include "Common/SceneRenderPass.h"

class CWaterStage : public CGraphicsPipelineStage
{
public:
	enum EPass
	{
		ePass_ReflectionGen = 0,
		ePass_FogVolume,
		ePass_WaterSurface,
		ePass_CausticsGen,

		ePass_Count,
	};
	static_assert(ePass_Count <= MAX_PIPELINE_SCENE_STAGE_PASSES, "Too many passes in one graphics pipeline stage");

	enum EPassMask
	{
		ePassMask_ReflectionGen = BIT(ePass_ReflectionGen),
		ePassMask_FogVolume     = BIT(ePass_FogVolume),
		ePassMask_WaterSurface  = BIT(ePass_WaterSurface),
		ePassMask_CausticsGen   = BIT(ePass_CausticsGen),
	};

	// default for water volume and ocean
	enum EPerInstanceTexture
	{
		ePerInstanceTexture_PerlinNoise = 14,
		ePerInstanceTexture_Jitter      = 15,

		ePerInstanceTexture_Count
	};

	enum EPerPassTexture
	{
		ePerPassTexture_WaterGloss            = 16,
		ePerPassTexture_Foam                  = 17,
		ePerPassTexture_RainRipple            = 18,

		ePerPassTexture_ShadoeMap0            = 19,
		ePerPassTexture_ShadoeMap1            = 20,
		ePerPassTexture_ShadoeMap2            = 21,
		ePerPassTexture_ShadoeMap3            = 22,

		ePerPassTexture_VolFogShadow          = 23,
		ePerPassTexture_VolumetricFog         = 24,
		ePerPassTexture_VolFogGlobalEnvProbe0 = 25,
		ePerPassTexture_VolFogGlobalEnvProbe1 = 26,

		ePerPassTexture_WaterRipple           = 27,
		ePerPassTexture_WaterVolumeNormal     = 28,
		ePerPassTexture_SceneDepth            = 29,
		ePerPassTexture_Refraction            = 30,
		ePerPassTexture_Reflection            = 31,

		ePerPassTexture_Count
	};
	// NOTE: DXOrbis only supports 32 shader slots at this time, don't use t32 or higher if DXOrbis support is desired!
	static_assert(ePerPassTexture_Count <= 32, "Bind slot too high for DXOrbis");

	enum EPerPassSampler
	{
		ePerPassSampler_PointWrap = EFSS_MAX,
		ePerPassSampler_PointClamp,

		ePerPassSampler_Aniso16xClamp,
		ePerPassSampler_Aniso16xWrap,
		ePerPassSampler_LinearClampComp,

		ePerPassSampler_Count,
	};
	static_assert(ePerPassSampler_Count <= 16, "Too many sampler states in one graphics pipeline stage");

	static const uint32 RainRippleTexCount = 24;

public:
	static bool UpdateCausticsGrid(N3DEngineCommon::SCausticInfo& causticInfo, bool& bVertexUpdated, CRenderer* pRenderer);

public:
	CWaterStage();
	virtual ~CWaterStage();

	void                         Init() override;
	void                         Prepare(CRenderView* pRenderView) override;

	void                         ExecuteWaterVolumeCaustics();
	void                         ExecuteDeferredWaterVolumeCaustics(bool bTiledDeferredShading);
	void                         ExecuteDeferredOceanCaustics();
	void                         ExecuteWaterFogVolumeBeforeTransparent();
	void                         Execute();

	const CDeviceResourceSetPtr& GetDefaultPerInstanceResourceSet() const { return m_pDefaultPerInstanceResources; }

	bool                         CreatePipelineStates(uint32 passMask, DevicePipelineStatesArray& pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);
	bool                         CreatePipelineState(CDeviceGraphicsPSOPtr& outPSO, const SGraphicsPipelineStateDescription& desc, EPass passID, std::function<void(CDeviceGraphicsPSODesc& psoDesc)> modifier);

private:
	bool  PrepareResourceLayout();
	bool  PrepareDefaultPerInstanceResources();
	bool  PreparePerPassResources(CRenderView* RESTRICT_POINTER pRenderView, bool bOnInit, EPass passId);
	void  UpdatePerPassResources(CRenderView& renderView);

	void  ExecuteWaterVolumeCausticsGen(N3DEngineCommon::SCausticInfo& causticInfo, CRenderView* pRenderView);
	void  ExecuteReflection(CRenderView* pRenderView);

	void  ExecuteSceneRenderPass(CRenderView* pRenderView, CSceneRenderPass& pass, ERenderListID renderList);

	int32 GetCurrentFrameID(const int32 frameID) const;
	int32 GetPreviousFrameID(const int32 frameID) const;

private:
	CSceneRenderPass                          m_passWaterCausticsSrcGen;
	CFullscreenPass                           m_passWaterCausticsDilation;
	CGaussianBlurPass                         m_passBlurWaterCausticsGen0;
	CPrimitiveRenderPass                      m_passRenderCausticsGrid;
	CGaussianBlurPass                         m_passBlurWaterCausticsGen1;
	CFullscreenPass                           m_passDeferredWaterVolumeCaustics;
	CPrimitiveRenderPass                      m_passDeferredOceanCausticsStencil;
	CFullscreenPass                           m_passDeferredOceanCaustics;
	CSceneRenderPass                          m_passWaterFogVolumeBeforeWater;
	CStretchRectPass                          m_passCopySceneTargetReflection;
	CSceneRenderPass                          m_passWaterReflectionGen;
	CMipmapGenPass                            m_passMipmapGen;
	CStretchRectPass                          m_passCopySceneTarget;
	CSceneRenderPass                          m_passWaterSurface;
	CSceneRenderPass                          m_passWaterFogVolumeAfterWater;

	CDeviceResourceLayoutPtr                  m_pResourceLayout;
	CDeviceResourceSetPtr                     m_pDefaultPerInstanceResources;
	CDeviceResourceSetPtr                     m_pPerPassResources;
	CConstantBufferPtr                        m_pPerPassCB;

	CRenderPrimitive                          m_causticsGridPrimitive;
	CRenderPrimitive                          m_deferredOceanStencilPrimitive[2];

	CTexture*                                 m_pFoamTex;
	CTexture*                                 m_pPerlinNoiseTex;
	CTexture*                                 m_pJitterTex;
	CTexture*                                 m_pWaterGlossTex;
	CTexture*                                 m_pOceanWavesTex;
	CTexture*                                 m_pOceanCausticsTex;

	std::array<CTexture*, RainRippleTexCount> m_pRainRippleTex;

	uint32 m_rainRippleTexIndex;
};
