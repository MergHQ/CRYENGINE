// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/GraphicsPipelineStage.h"
#include "Common/GraphicsPipelineStateSet.h"
#include "Common/SceneRenderPass.h"


class CSceneForwardStage : public CGraphicsPipelineStage
{
	enum EPerPassTexture
	{
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
