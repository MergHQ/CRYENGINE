// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/SceneRenderPass.h"

struct SGraphicsPipelineStateDescription;

class CSceneCustomStage : public CGraphicsPipelineStage
{
	enum EPerPassTexture
	{
		ePerPassTexture_SceneDepthBuffer = 25,
		ePerPassTexture_TerrainElevMap = 26,
		ePerPassTexture_WindGrid,
		ePerPassTexture_TerrainNormMap,
		ePerPassTexture_TerrainBaseMap,
		ePerPassTexture_NormalsFitting,
		ePerPassTexture_DissolveNoise,

		ePerPassTexture_Count
	};

	// NOTE: DXOrbis only supports 32 shader slots at this time, don't use t32 or higher if DXOrbis support is desired!
	static_assert(ePerPassTexture_Count <= 32, "Bind slot too high for DXOrbis");
	
	enum EPass
	{
		ePass_Wireframe   = 0,
		ePass_SelectionIDs, // draw highlighted objects from editor
	};

public:
	virtual void Init() override;

	void Execute();

	bool CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);

private:
	bool CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO);
	bool PreparePerPassResources(bool bOnInit);

private:
	CDeviceResourceSetPtr    m_pPerPassResources;
	CDeviceResourceLayoutPtr m_pResourceLayout;
	
	CSceneRenderPass         m_wireframePass;
	CSceneRenderPass         m_selectionIDPass;

	CFullscreenPass          m_highlightPass;

	SDepthTexture            m_depthTarget;

	int                      m_samplerPoint;
	int                      m_samplerLinear;
};