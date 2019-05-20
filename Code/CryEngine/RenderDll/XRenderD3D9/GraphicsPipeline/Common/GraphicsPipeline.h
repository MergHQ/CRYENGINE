// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/RenderView.h"
#include "Common/TypedConstantBuffer.h"
#include "GraphicsPipelineStateSet.h"
#include "RenderPassScheduler.h"

enum EGraphicsPipelineStage
{
	// Scene stages
	eStage_ShadowMap,
	eStage_SceneGBuffer,
	eStage_SceneForward,
	eStage_SceneCustom,
	eStage_SCENE_NUM,

	// Regular stages supporting async compute
	eStage_FIRST_ASYNC_COMPUTE = eStage_SCENE_NUM,
	eStage_ComputeSkinning     = eStage_FIRST_ASYNC_COMPUTE,
	eStage_ComputeParticles,
	eStage_TiledLightVolumes,
	eStage_TiledShading,
	eStage_VolumetricClouds,

	// Regular stages
	eStage_SceneDepth, // TODO: pure compute
	eStage_HeightMapAO,
	eStage_ScreenSpaceObscurance,
	eStage_ScreenSpaceReflections,
	eStage_ScreenSpaceSSS,
	eStage_VolumetricFog,
	eStage_Fog,
	eStage_Sky,
	eStage_WaterRipples,
	eStage_Water,
	eStage_MotionBlur,
	eStage_DepthOfField,
	eStage_AutoExposure,
	eStage_Bloom,
	eStage_ColorGrading,
	eStage_ToneMapping,
	eStage_Sunshafts,
	eStage_PostAA,
	eStage_ClipVolumes,
	eStage_DeferredDecals,
	eStage_ShadowMask,
	eStage_LensOptics,
	eStage_PostEffect,
	eStage_Rain,
	eStage_Snow,
	eStage_MobileComposition,
	eStage_OmniCamera,
	eStage_DebugRenderTargets,

	eStage_Count
};

class CGraphicsPipelineStage;
struct SRenderViewInfo;

class CGraphicsPipeline
{
public:
	enum class EPipelineFlags : uint64
	{
		NO_SHADER_FOG = BIT(0),
	};

public:
	CGraphicsPipeline(const IRenderer::SGraphicsPipelineDescription& desc, const std::string& uniqueIdentifier, const SGraphicsPipelineKey key);

	virtual ~CGraphicsPipeline();

	// Convention for used verbs:
	//
	//  Get........(): mostly const functions to retreive resource-references
	//  Generate...(): CPU side data generation (may also update upload buffers or GPU side buffers)
	//                 This could produce Copy() commands on a command-list (context free from the core CL)
	//  Prepare....(): Put all the resources used by the corresponding Execute() into the desired state
	//                 This produces a lot of barriers and other commands on the core CL or maybe compute CLs
	//  Execute....(): Recording of the GPU side commands on the core CL or maybe compute CLs

	// Allocate resources needed by the pipeline and its stages
	virtual void Init();
	virtual void Resize(int renderWidth, int renderHeight);
	// Prepare all stages before actual drawing starts
	virtual void Update(EShaderRenderingFlags renderingFlags);
	virtual void OnCVarsChanged(const CCVarUpdateRecorder& rCVarRecs);
	// Execute the pipeline and its stages
	virtual void Execute() = 0;

	virtual bool IsInitialized() const { return m_bInitialized; }

	virtual void ShutDown();

	void GenerateMainViewConstantBuffer()
	{
		GeneratePerViewConstantBuffer(&m_pCurrentRenderView->GetViewInfo(CCamera::eEye_Left), m_pCurrentRenderView->GetViewInfoCount(), m_mainViewConstantBuffer.GetDeviceConstantBuffer());
	}
	size_t GetViewInfoCount() const
	{
		return m_pCurrentRenderView->GetViewInfoCount();
	}
	size_t GenerateViewInfo(SRenderViewInfo viewInfo[2])
	{
		for (int i = 0; i < m_pCurrentRenderView->GetViewInfoCount(); i++)
		{
			viewInfo[i] = m_pCurrentRenderView->GetViewInfo((CCamera::EEye)i);
		}
		return m_pCurrentRenderView->GetViewInfoCount();
	}

	virtual bool CreatePipelineStates(DevicePipelineStatesArray* pStateArray,
	                                  SGraphicsPipelineStateDescription stateDesc,
	                                  CGraphicsPipelineStateLocalCache* pStateCache) = 0;

	virtual bool                     FillCommonScenePassStates(const SGraphicsPipelineStateDescription& inputDesc, CDeviceGraphicsPSODesc& psoDesc) = 0;
	virtual CDeviceResourceLayoutPtr CreateScenePassLayout(const CDeviceResourceSetDesc& perPassResources) = 0;

public:
	bool                  AllowsHDRRendering() const                            { return (m_renderingFlags & SHDF_ALLOWHDR) != 0; }
	bool                  IsPostProcessingEnabled() const                       { return (m_renderingFlags & SHDF_ALLOWPOSTPROCESS) && CRenderer::IsPostProcessingEnabled(); }

	CRenderPassScheduler& GetRenderPassScheduler()                              { return m_renderPassScheduler; }

	void                  SetVrProjectionManager(CVrProjectionManager* manager) { m_pVRProjectionManager = manager; }
	CVrProjectionManager* GetVrProjectionManager() const                        { return m_pVRProjectionManager; }

	void                  SetCurrentRenderView(CRenderView* pRenderView);
	CRenderView*          GetCurrentRenderView()    const { return m_pCurrentRenderView; }
	CRenderOutput*        GetCurrentRenderOutput()  const { return m_pCurrentRenderView->GetRenderOutput(); }

	//! Set current pipeline flags
	void           SetPipelineFlags(EPipelineFlags pipelineFlags)           { m_pipelineFlags = pipelineFlags; }
	//! Get current pipeline flags
	EPipelineFlags GetPipelineFlags() const                                 { return m_pipelineFlags; }
	bool           IsPipelineFlag(EPipelineFlags pipelineFlagsToTest) const { return 0 != ((uint64)m_pipelineFlags & (uint64)pipelineFlagsToTest); }

	// Animation time is used for rendering animation effects and can be paused if CRenderer::m_bPauseTimer is true
	CTimeValue GetAnimationTime() const { return gRenDev->GetAnimationTime(); }

	// Time of the last main to renderer thread sync
	CTimeValue                                     GetFrameSyncTime() const       { return gRenDev->GetFrameSyncTime(); }

	const IRenderer::SGraphicsPipelineDescription& GetPipelineDescription() const { return m_pipelineDesc; }

	uint32                                         GetRenderFlags() const         { return m_renderingFlags; }

	const Vec2_tpl<uint32_t>                       GetRenderResolution() const    { return Vec2_tpl<uint32_t>(m_renderWidth, m_renderHeight); };

	const SRenderViewInfo&                         GetCurrentViewInfo(CCamera::EEye eye) const;

	const SGraphicsPipelineKey                     GetKey() const                             { return m_key; }
	void                                           GeneratePerViewConstantBuffer(const SRenderViewInfo* pViewInfo, int viewInfoCount, CConstantBufferPtr pPerViewBuffer, const SRenderViewport* pCustomViewport = nullptr);
	uint32                                         IncrementNumInvalidDrawcalls(int count)    { return CryInterlockedAdd((volatile int*)&m_numInvalidDrawcalls, count); }
	uint32                                         GetNumInvalidDrawcalls() const             { return m_numInvalidDrawcalls; }

	CConstantBufferPtr                             GetMainViewConstantBuffer()                { return m_mainViewConstantBuffer.GetDeviceConstantBuffer(); }
	const CDeviceResourceSetDesc&                  GetDefaultMaterialBindPoints()       const { return m_defaultMaterialBindPoints; }
	std::array<SamplerStateHandle, EFSS_MAX>       GetDefaultMaterialSamplers()         const;
	const CDeviceResourceSetDesc&                  GetDefaultDrawExtraResourceLayout()  const { return m_defaultDrawExtraRL; }
	CDeviceResourceSetPtr                          GetDefaulDrawExtraResourceSet()      const { return m_pDefaultDrawExtraRS; }

	const std::string&                             GetUniqueIdentifierName() const            { return m_uniquePipelineIdentifierName; }

	CLightVolumeBuffer&                            GetLightVolumeBuffer()                     { return m_lightVolumeBuffer; }

	void                                           SetParticleBuffers(bool bOnInit, CDeviceResourceSetDesc& resources, ResourceViewHandle hView, EShaderStage shaderStages) const;

	// Partial pipeline functions, will be removed once the entire pipeline is implemented in Execute()
	void                                           ExecutePostAA();
	void                                           ExecuteAnisotropicVerticalBlur(CTexture* pTex, int nAmount, float fScale, float fDistribution, bool bAlphaOnly);

	static void                                    ApplyShaderQuality(CDeviceGraphicsPSODesc& psoDesc, const SShaderProfile& shaderProfile);

	void                                           ClearState();
	void                                           ClearDeviceState();

	template<class T> T* GetStage() { return static_cast<T*>(m_pipelineStages[T::StageID]); }

protected:

	template<class T> void RegisterStage()
	{
		T* pPipelineStage = new T(*this);
		m_pipelineStages[T::StageID] = pPipelineStage;
	}

public:
	std::unique_ptr<CStretchRectPass>             m_ResolvePass;
	std::unique_ptr<CDownsamplePass>              m_DownscalePass;
	std::unique_ptr<CSharpeningUpsamplePass>      m_UpscalePass;
	std::unique_ptr<CAnisotropicVerticalBlurPass> m_AnisoVBlurPass;

protected:
	std::array<CGraphicsPipelineStage*, eStage_Count>           m_pipelineStages;
	CRenderPassScheduler                                        m_renderPassScheduler;

	CDeviceResourceSetDesc                                      m_defaultMaterialBindPoints;
	CDeviceResourceSetDesc                                      m_defaultDrawExtraRL;
	CDeviceResourceSetPtr                                       m_pDefaultDrawExtraRS;

	EShaderRenderingFlags                                       m_renderingFlags = EShaderRenderingFlags(0);

	CTypedConstantBuffer<HLSL_PerViewGlobalConstantBuffer, 256> m_mainViewConstantBuffer;

	CVrProjectionManager*                                       m_pVRProjectionManager;

	bool m_bInitialized = false;

	int  m_numInvalidDrawcalls = 0;

private:
	CRenderView* m_pCurrentRenderView = nullptr;

	// light volume data
	CLightVolumeBuffer m_lightVolumeBuffer;

	//! @see EPipelineFlags
	EPipelineFlags                          m_pipelineFlags;

	uint32_t                                m_renderWidth = 0;
	uint32_t                                m_renderHeight = 0;

	std::string                             m_uniquePipelineIdentifierName;

	IRenderer::SGraphicsPipelineDescription m_pipelineDesc;
	SGraphicsPipelineKey                    m_key = SGraphicsPipelineKey::InvalidGraphicsPipelineKey;
};

DEFINE_ENUM_FLAG_OPERATORS(CGraphicsPipeline::EPipelineFlags)
