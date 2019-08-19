// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/FullscreenPass.h"
#include "Common/UtilityPasses.h"
#include "Common/PrimitiveRenderPass.h"

class CRenderView;
class CEffectParam;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CPostEffectContext : private NoCopy
{
public:
	CPostEffectContext();
	virtual ~CPostEffectContext();

	void         Setup(CPostEffectsMgr* pPostEffectsMgr);

	void         EnableAltBackBuffer(bool enable);

	void         SetRenderView(CRenderView* pRenderView) { m_pRenderView = pRenderView; }
	CRenderView* GetRenderView() const                   { return m_pRenderView; }

public:
	uint64              GetShaderRTMask() const;

	CTexture*           GetSrcBackBufferTexture() const;
	CTexture*           GetDstBackBufferTexture() const;
	CTexture*           GetDstDepthStencilTexture() const;

	CPostEffect*        GetPostEffect(EPostEffectID nID) const;
	const CEffectParam* GetEffectParamByName(const char* pszParam) const;

private:
	CPostEffectsMgr* m_pPostEffectsMgr = nullptr;
	uint64           m_shaderRTMask = 0;
	bool             m_bUseAltBackBuffer = false;
	CRenderView*     m_pRenderView = nullptr;
};

struct IPostEffectPass : private NoCopy
{
	virtual ~IPostEffectPass() {}

	virtual void Init(CPostEffectContext* p) = 0;
	virtual void Update() {}
	virtual void Resize(int renderWidth, int renderHeight) {}
	virtual void Execute() = 0;

	CPostEffectContext* m_pContext;
};

class CPostEffectStage : public CGraphicsPipelineStage
{
public:
	static const EGraphicsPipelineStage StageID = eStage_PostEffect;

	CPostEffectStage(CGraphicsPipeline& graphicsPipeline)
		: CGraphicsPipelineStage(graphicsPipeline)
		, m_passCopyScreenToTex(&graphicsPipeline)
	{
		m_pPostMgr = nullptr;
	}

	virtual ~CPostEffectStage() {}

	bool IsStageActive(EShaderRenderingFlags flags) const
	{
		if (!CRenderer::CV_r_PostProcess) return false;
		if (!CShaderMan::s_shPostEffects) return false;
		if (!m_pPostMgr || m_pPostMgr->GetEffects().empty()) return false;

		if (RenderView()->IsRecursive()) return false;

		// Skip hdr/post processing when rendering different camera views
		if ((RenderView()->IsViewFlag(SRenderViewInfo::eFlags_MirrorCull)) ||
			(flags & SHDF_CUBEMAPGEN) || 
			(flags & SHDF_ALLOWPOSTPROCESS) == 0)
		{
			return false;
		}

		return true;
	}

	void Init() final;
	void Resize(int renderWidth, int renderHeight) final;
	void Update() final;

	void Execute();

	void OnCVarsChanged(const CCVarUpdateRecorder& cvarUpdater) final;

private:
	void Execute3DHudFlashUpdate();

private:
	CPostEffectsMgr*   m_pPostMgr;
	std::array<std::unique_ptr<IPostEffectPass>, EPostEffectID::Num> m_postEffectArray;
	CPostEffectContext m_context;

	CStretchRectPass   m_passCopyScreenToTex;
#ifndef _RELEASE
	CFullscreenPass    m_passAntialiasingDebug;
#endif
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CUnderwaterGodRaysPass : public IPostEffectPass
{
public:
	CUnderwaterGodRaysPass(CGraphicsPipeline* pGraphicsPipeline) : m_passUnderwaterGodRaysFinal(pGraphicsPipeline)
	{
		for (auto& pass : m_passUnderwaterGodRaysGen)
		{
			pass.SetGraphicsPipeline(pGraphicsPipeline);
		}
	}
	~CUnderwaterGodRaysPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	static const int32 SliceCount = 10;

private:
	CFullscreenPass m_passUnderwaterGodRaysGen[SliceCount];
	CFullscreenPass m_passUnderwaterGodRaysFinal;

	CTexture*       m_pWavesTex = nullptr;
	CTexture*       m_pCausticsTex = nullptr;
	CTexture*       m_pUnderwaterBumpTex = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class CWaterDropletsPass : public IPostEffectPass
{
public:
	CWaterDropletsPass(CGraphicsPipeline* pGraphicsPipeline) : m_passWaterDroplets(pGraphicsPipeline) {}
	~CWaterDropletsPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passWaterDroplets;

	CTexture*       m_pWaterDropletsBumpTex = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class CWaterFlowPass : public IPostEffectPass
{
public:
	CWaterFlowPass(CGraphicsPipeline* pGraphicsPipeline) : m_passWaterFlow(pGraphicsPipeline) {}
	~CWaterFlowPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passWaterFlow;

	CTexture*       m_pWaterFlowBumpTex = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class CSharpeningPass : public IPostEffectPass
{
public:
	CSharpeningPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passStrechRect(pGraphicsPipeline)
		, m_passSharpeningAndChromaticAberration(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CStretchRectPass m_passStrechRect;
	CFullscreenPass  m_passSharpeningAndChromaticAberration;
};

//////////////////////////////////////////////////////////////////////////
class CBlurringPass : public IPostEffectPass
{
public:
	CBlurringPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passStrechRect(pGraphicsPipeline)
		, m_passGaussianBlur(pGraphicsPipeline)
		, m_passBlurring(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CStretchRectPass  m_passStrechRect;
	CGaussianBlurPass m_passGaussianBlur;
	CFullscreenPass   m_passBlurring;
};

//////////////////////////////////////////////////////////////////////////
class CUberGamePostEffectPass : public IPostEffectPass
{
public:
	CUberGamePostEffectPass(CGraphicsPipeline* pGraphicsPipeline) : m_passRadialBlurAndChromaShift(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passRadialBlurAndChromaShift;
};

//////////////////////////////////////////////////////////////////////////
class CFlashBangPass : public IPostEffectPass
{
public:
	CFlashBangPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passStrechRect(pGraphicsPipeline)
		, m_passFlashBang(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CStretchRectPass m_passStrechRect;
	CFullscreenPass  m_passFlashBang;
};

//////////////////////////////////////////////////////////////////////////
class CPostStereoPass : public IPostEffectPass
{
public:
	CPostStereoPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passNearMask(pGraphicsPipeline)
		, m_passPostStereo(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass    m_passNearMask;
	CFullscreenPass    m_passPostStereo;

	SamplerStateHandle m_samplerLinearMirror = EDefaultSamplerStates::Unspecified;
};

//////////////////////////////////////////////////////////////////////////
class CKillCameraPass : public IPostEffectPass
{
public:
	CKillCameraPass(CGraphicsPipeline* pGraphicsPipeline) : m_passKillCameraFilter(pGraphicsPipeline) {}
	~CKillCameraPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passKillCameraFilter;

	CTexture*       m_pNoiseTex = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class CScreenBloodPass : public IPostEffectPass
{
public:
	CScreenBloodPass(CGraphicsPipeline* pGraphicsPipeline) : m_passScreenBlood(pGraphicsPipeline) {}
	~CScreenBloodPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passScreenBlood;

	CTexture*       m_pWaterDropletsBumpTex = nullptr;
};

//////////////////////////////////////////////////////////////////////////
class CScreenFaderPass : public IPostEffectPass
{
public:
	CScreenFaderPass(CGraphicsPipeline* pGraphicsPipeline) : m_passScreenFader(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	CFullscreenPass m_passScreenFader;
};

//////////////////////////////////////////////////////////////////////////
class CHudSilhouettesPass : public IPostEffectPass
{
public:
	CHudSilhouettesPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passStrechRect(pGraphicsPipeline)
		, m_passDeferredSilhouettesOptimised(pGraphicsPipeline) {}

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

private:
	void ExecuteDeferredSilhouettesOptimised(const CPostEffectContext& context, float fBlendParam, float fType, float fFillStrength);

private:
	CStretchRectPass m_passStrechRect;
	CFullscreenPass  m_passDeferredSilhouettesOptimised;
};

//////////////////////////////////////////////////////////////////////////
class CHud3DPass : public IPostEffectPass
{
public:
	CHud3DPass(CGraphicsPipeline* pGraphicsPipeline)
		: m_passBlurGaussian(pGraphicsPipeline) {}

	~CHud3DPass();

	virtual void Init(CPostEffectContext* p) final;
	virtual void Execute() final;

	void ExecuteFlashUpdate(class CHud3D & hud3d);

private:
	void ExecuteDownsampleHud4x4(class CHud3D & hud3d, CTexture * pDstRT);
	void ExecuteBloomTexUpdate(class CHud3D & hud3d);
	void ExecuteFinalPass(CTexture* pOutputRT, CTexture* pOutputDS, class CHud3D & hud3d);

	bool SetVertex(CRenderPrimitive& prim, struct SHudData& pData) const;
	void SetShaderParams(EShaderStage shaderStages, CRenderPrimitive::ConstantManager& constantManager, const struct SHudData& data, const class CHud3D& hud3d) const;

private:
	CPrimitiveRenderPass          m_passDownsampleHud4x4;
	CPrimitiveRenderPass          m_passUpdateBloom;
	CGaussianBlurPass             m_passBlurGaussian;
	CPrimitiveRenderPass          m_passRenderHud;

	std::vector<CRenderPrimitive*> m_downsamplePrimitiveArray;
	std::vector<CRenderPrimitive*> m_bloomPrimitiveArray;
	std::vector<CRenderPrimitive*> m_hudPrimitiveArray;
};
