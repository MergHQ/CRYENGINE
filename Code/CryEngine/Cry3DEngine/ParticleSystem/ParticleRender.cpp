// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     26/11/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ParticleRender.h"
#include "ParticleManager.h"

namespace pfx2
{

CParticleRenderBase::CParticleRenderBase(gpu_pfx2::EGpuFeatureType type)
	: CParticleFeature(type)
{
}

EFeatureType CParticleRenderBase::GetFeatureType()
{
	return EFT_Render;
}

void CParticleRenderBase::AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams)
{
	pComponent->AddToUpdateList(EUL_Render, this);
}

void CParticleRenderBase::Render(ICommonParticleComponentRuntime* pComponentRuntime, CParticleComponent* pComponent, IRenderNode* pNode, const SRenderContext& renderContext)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	const SComponentParams& params = pComponent->GetComponentParams();

	if (!params.m_pMaterial)
		return;

	uint64 objFlags = params.m_renderObjectFlags;
	if (renderContext.m_lightVolumeId)
		objFlags |= FOB_LIGHTVOLUME;

	if (SupportsWaterCulling())
		AddRenderObject(pComponentRuntime, pComponent, renderContext, pNode, objFlags);
	AddRenderObject(pComponentRuntime, pComponent, renderContext, pNode, objFlags | FOB_AFTER_WATER);
}

void CParticleRenderBase::AddRenderObject(ICommonParticleComponentRuntime* pComponentRuntime, CParticleComponent* pComponent, const SRenderContext& renderContext, IRenderNode* pNode, uint64 objFlags)
{
	const SComponentParams& params = pComponent->GetComponentParams();
	const SParticleShaderData& rData = params.m_shaderData;

	C3DEngine* p3DEngine = Get3DEngine();
	threadID passThreadId = renderContext.m_passInfo.ThreadID();

	const float sortBiasSize = 1.0f / 1024.0f;
	const float sortBias = renderContext.m_distance * params.m_renderObjectSortBias * sortBiasSize;
	CRenderObject* pRenderObject = gEnv->pRenderer->EF_GetObject_Temp(passThreadId);
	pRenderObject->m_II.m_Matrix.SetIdentity();
	pRenderObject->m_ObjFlags = objFlags;
	pRenderObject->m_fAlpha = 1.0f;
	pRenderObject->m_pCurrMaterial = params.m_pMaterial;
	pRenderObject->m_pRenderNode = pNode;
	pRenderObject->m_RState = params.m_renderStateFlags | OS_ENVIRONMENT_CUBEMAP;
	pRenderObject->m_fSort = 0;
	pRenderObject->m_fDistance = renderContext.m_distance - sortBias;
	pRenderObject->m_ParticleObjFlags = params.m_particleObjFlags;

	SRenderObjData* pObjData = pRenderObject->GetObjData();
	pObjData->m_pParticleShaderData = &params.m_shaderData;
	pObjData->m_FogVolumeContribIdx = renderContext.m_fogVolumeId;
	pObjData->m_LightVolumeId = renderContext.m_lightVolumeId;
	if (pNode->m_pTempData)
		*((Vec4*)&pObjData->m_fTempVars[0]) = pNode->m_pTempData->userData.vEnvironmentProbeMults;
	else
		*((Vec4*)&pObjData->m_fTempVars[0]) = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (auto pGpuRuntime = pComponentRuntime->GetGpuRuntime())
	{
		pGpuRuntime->Render(pRenderObject, renderContext.m_passInfo, renderContext.m_renderParams);
	}
	else
	{
		CParticleJobManager& kernel = GetPSystem()->GetJobManager();
		kernel.ScheduleComputeVertices(pComponentRuntime->GetCpuRuntime(), pRenderObject, renderContext);
	}
}

}
