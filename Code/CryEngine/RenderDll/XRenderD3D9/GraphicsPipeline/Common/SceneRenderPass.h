// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CSceneRenderPass
{
public:
	enum EPassFlags
	{
		ePassFlags_None          = 0,
		ePassFlags_RenderNearest = BIT(0),
		ePassFlags_ReverseDepth  = BIT(1),
	};

	CSceneRenderPass();

	void SetupPassContext(uint32 stageID, uint32 stagePassID, EShaderTechniqueID technique, uint32 filter);
	void SetPassResources(CDeviceResourceLayoutPtr pResourceLayout, CDeviceResourceSetPtr pPerPassResources);
	void SetRenderTargets(SDepthTexture* pDepthTarget, CTexture* pColorTarget0, CTexture* pColorTarget1 = NULL, CTexture* pColorTarget2 = NULL, CTexture* pColorTarget3 = NULL);
	void ExchangeRenderTarget(uint32 slot, CTexture* pNewColorTarget);
	void SetLabel(const char* label) { m_szLabel = label; }
	void SetFlags(EPassFlags flags)  { m_passFlags = flags; }
	void SetViewport(const D3DViewPort& viewport);
	void EnableNearestViewport(bool bNearest);

	void ExtractRenderTargetFormats(CDeviceGraphicsPSODesc& psoDesc);

	void DrawRenderItems(CRenderView* pRenderView, ERenderListID list, int listStart = -1, int listEnd = -1, int profilingListID = -1);

	// Called from rendering backend (has to be threadsafe)
	void               PrepareRenderPassForUse(CDeviceCommandListRef RESTRICT_REFERENCE commandList);
	void               BeginRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList);
	void               EndRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList);

	uint32             GetStageID()        const { return m_stageID; }
	uint32             GetPassID()         const { return m_passID; }
	EPassFlags         GetFlags()          const { return m_passFlags; }
	const D3DViewPort& GetViewport()       const { return m_viewPort[m_bNearestViewport]; }
	const D3DViewPort& GetViewport(bool n) const { return m_viewPort[n]; }

protected:
	void DrawRenderItems_GP2(SGraphicsPipelinePassContext& passContext);

protected:
	SDepthTexture*           m_pDepthTarget;
	CTexture*                m_pColorTargets[4];
	D3DViewPort              m_viewPort[2];
	D3DRectangle             m_scissorRect;
	CDeviceResourceLayoutPtr m_pResourceLayout;
	CDeviceResourceSetPtr    m_pPerPassResources;
	bool                     m_bNearestViewport;
	const char*              m_szLabel;

	EShaderTechniqueID       m_technique;
	uint32                   m_stageID : 16;
	uint32                   m_passID  : 16;
	uint32                   m_batchFilter;
	EPassFlags               m_passFlags;

	SProfilingStats          m_profilingStats;
};

DEFINE_ENUM_FLAG_OPERATORS(CSceneRenderPass::EPassFlags)
