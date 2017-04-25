// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/SceneRenderPass.h"
#include "Common/FullscreenPass.h"

struct SGraphicsPipelineStateDescription;

class CSceneCustomStage : public CGraphicsPipelineStage
{
	enum EPerPassTexture
	{
		ePerPassTexture_SceneLinearDepth = 24,
		ePerPassTexture_PerlinNoiseMap,
		ePerPassTexture_TerrainElevMap,
		ePerPassTexture_WindGrid,
		ePerPassTexture_TerrainNormMap,
		ePerPassTexture_TerrainBaseMap,
		ePerPassTexture_NormalsFitting,
		ePerPassTexture_DissolveNoise,
		ePerPassTexture_SceneDepthBuffer,

		ePerPassTexture_Count
	};
	
	enum EPass
	{
		ePass_DebugViewSolid = 0,
		ePass_DebugViewWireframe,
		ePass_SelectionIDs, // draw highlighted objects from editor
		ePass_DebugDraw,
		ePass_Silhouette,
	};

public:
	virtual void Init() override;

	void Execute();
	void ExecuteSilhouettePass();

	bool CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);

private:
	bool CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO);
	bool PreparePerPassResources(bool bOnInit);

private:
	CDeviceResourceSetPtr    m_pPerPassResources;
	CDeviceResourceLayoutPtr m_pResourceLayout;
	CConstantBufferPtr       m_pPerPassConstantBuffer;
	
	CSceneRenderPass         m_debugViewPass;
	CSceneRenderPass         m_debugDrawPass;

	CSceneRenderPass         m_selectionIDPass;
	CFullscreenPass          m_highlightPass;

	CSceneRenderPass         m_silhouetteMaskPass;

	SDepthTexture            m_depthTarget;

	int                      m_samplerPoint;
	int                      m_samplerLinear;
};
