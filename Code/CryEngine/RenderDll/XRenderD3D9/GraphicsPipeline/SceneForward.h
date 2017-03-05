// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/SceneRenderPass.h"


class CSceneForwardStage : public CGraphicsPipelineStage
{
	enum EPerPassTexture
	{
		ePerPassTexture_PerlinNoiseMap = 25,
		ePerPassTexture_TerrainElevMap,
		ePerPassTexture_WindGrid,
		ePerPassTexture_TerrainNormMap,
		ePerPassTexture_TerrainBaseMap,
		ePerPassTexture_NormalsFitting,
		ePerPassTexture_DissolveNoise,

		ePerPassTexture_Count
	};
	
	enum EPass
	{
		ePass_Forward  = 0
	};

public:
	virtual void Init() override;

	bool         CreatePipelineStates(DevicePipelineStatesArray* pStateArray, const SGraphicsPipelineStateDescription& stateDesc, CGraphicsPipelineStateLocalCache* pStateCache);

	void         Execute_Opaque();
	void         Execute_TransparentBelowWater();
	void         Execute_TransparentAboveWater();

private:
	bool CreatePipelineState(const SGraphicsPipelineStateDescription& desc, EPass passID, CDeviceGraphicsPSOPtr& outPSO);
	bool PreparePerPassResources(bool bOnInit);

private:
	CDeviceResourceLayoutPtr m_pResourceLayout;

	CDeviceResourceSetPtr    m_pOpaquePassResources;
	CDeviceResourceSetPtr    m_pTransparentPassResources;
	
	CSceneRenderPass         m_forwardOpaquePass;
};
