// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryRenderer/RenderElements/CREParticle.h>
#include <CryCore/Containers/HeapContainer.h>
#include "../ParticleBuffer.h"

#include <iterator>
#include <Cry3DEngine/I3DEngine.h>
#include <CryParticleSystem/IParticles.h>
#include <CryThreading/IJobManager_JobDelegator.h>
#include "Common/RenderView.h"
#include "CompiledRenderObject.h"

DECLARE_JOB("ComputeVertices", TComputeVerticesJob, CREParticle::ComputeVertices);

//////////////////////////////////////////////////////////////////////////
// CFillRateManager implementation

void CFillRateManager::AddPixelCount(float fPixels)
{
	if (fPixels > 0.f)
	{
		Lock();
		m_afPixels.push_back(fPixels);
		m_fTotalPixels += fPixels;
		Unlock();
	}
}

void CFillRateManager::ComputeMaxPixels()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	// Find per-container maximum which will not exceed total.
	// don't use static here, this function can be called before particle cvars are registered
	ICVar* pVar = gEnv->pConsole->GetCVar("e_ParticlesMaxScreenFill");
	if (!pVar)
		return;
	float fMaxTotalPixels = pVar->GetFVal() * gRenDev->GetWidth() * gRenDev->GetHeight();
	float fNewMax = fMaxTotalPixels;

	Lock();

	if (m_fTotalPixels > fMaxTotalPixels)
	{
		// Compute max pixels we can have per emitter before total exceeded,
		// from previous frame's data.
		std::sort(m_afPixels.begin(), m_afPixels.end());
		float fUnclampedTotal = 0.f;
		int nRemaining = m_afPixels.size();
		for (auto& pixels : m_afPixels)
		{
			float fTotal = fUnclampedTotal + nRemaining * pixels;
			if (fTotal > fMaxTotalPixels)
			{
				fNewMax = (fMaxTotalPixels - fUnclampedTotal) / nRemaining;
				break;
			}
			fUnclampedTotal += pixels;
			nRemaining--;
		}
	}

	// Update current value gradually.
	float fLastMax = m_fMaxPixels;
	float fMaxChange = max(fLastMax, fNewMax) * 0.25f;
	m_fMaxPixels = clamp_tpl(fNewMax, fLastMax - fMaxChange, fLastMax + fMaxChange);

	Reset();

	Unlock();
}

//////////////////////////////////////////////////////////////////////////
//
// CREParticle implementation.
//

CREParticle::CREParticle()
	: m_pVertexCreator(0)
	, m_nFirstVertex(0)
	, m_nFirstIndex(0)
	, m_addedToView(0)
{
	mfSetType(eDATA_Particle);
}

void CREParticle::Reset(IParticleVertexCreator* pVC, int nThreadId, uint allocId)
{
	m_pVertexCreator = pVC;
	m_nThreadId = nThreadId;
	m_allocId = allocId;
	Construct(m_RenderVerts);
	m_nFirstVertex = 0;
	m_nFirstIndex = 0;
}

SRenderVertices* CREParticle::AllocVertices(int nAllocVerts, int nAllocInds)
{
	SRenderPipeline& rp = gRenDev->m_RP;

	CParticleBufferSet::SAlloc alloc;

	rp.m_particleBuffer.Alloc(m_allocId, CParticleBufferSet::EBT_Vertices, nAllocVerts, &alloc);
	SVF_Particle* pVertexBuffer = alias_cast<SVF_Particle*>(alloc.m_pBase) + alloc.m_firstElem;
	m_RenderVerts.aVertices.set(ArrayT(pVertexBuffer, int(alloc.m_numElemns)));
	m_nFirstVertex = alloc.m_firstElem;

	rp.m_particleBuffer.Alloc(m_allocId, CParticleBufferSet::EBT_Indices, nAllocInds, &alloc);
	uint16* pIndexBuffer = alias_cast<uint16*>(alloc.m_pBase) + alloc.m_firstElem;
	m_RenderVerts.aIndices.set(ArrayT(pIndexBuffer, int(alloc.m_numElemns)));
	m_nFirstIndex = alloc.m_firstElem;

	m_RenderVerts.fPixels = 0.f;

	return &m_RenderVerts;
}

SRenderVertices* CREParticle::AllocPullVertices(int nPulledVerts)
{
	SRenderPipeline& rp = gRenDev->m_RP;

	CParticleBufferSet::SAllocStreams streams;
	rp.m_particleBuffer.Alloc(m_allocId, nPulledVerts, &streams);
	m_RenderVerts.aPositions.set(ArrayT(streams.m_pPositions, int(streams.m_numElemns)));
	m_RenderVerts.aAxes.set(ArrayT(streams.m_pAxes, int(streams.m_numElemns)));
	m_RenderVerts.aColorSTs.set(ArrayT(streams.m_pColorSTs, int(streams.m_numElemns)));
	m_nFirstVertex = streams.m_firstElem;

	m_RenderVerts.fPixels = 0.f;

	return &m_RenderVerts;
}

void CREParticle::mfPrepare(bool bCheckOverflow)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	CRenderer* rd = gRenDev;
	SRenderPipeline& rRP = rd->m_RP;
	CRenderObject* pRenderObject = rRP.m_pCurObject;

	const bool isPulledVertices = (pRenderObject->m_ParticleObjFlags & CREParticle::ePOF_USE_VERTEX_PULL_MODEL) != 0;

	rRP.m_CurVFormat = isPulledVertices ? eVF_Unknown : eVF_P3F_C4B_T4B_N3F2;

	rd->FX_StartMerging();

	// the update jobs are synced in the render thread, before working on the particle-batch
	// tell the render pipeline how many vertices/indices we computed and that we are
	// using our own extern video memory buffer
	rRP.m_RendNumVerts += m_RenderVerts.aVertices.size() + m_RenderVerts.aPositions.size();
	rRP.m_RendNumIndices += m_RenderVerts.aIndices.size();
	rRP.m_FirstIndex = m_nFirstIndex;
	rRP.m_FirstVertex = m_nFirstVertex;
	rRP.m_PS[rRP.m_nProcessThreadID].m_DynMeshUpdateBytes += rRP.m_StreamStride * rRP.m_RendNumVerts;
	rRP.m_PS[rRP.m_nProcessThreadID].m_DynMeshUpdateBytes += rRP.m_RendNumIndices * sizeof(uint16);

	gRenDev->m_FillRateManager.AddPixelCount(m_RenderVerts.fPixels);

	if (rRP.m_RendNumVerts)
		rRP.m_pRE = this;

#if !defined(_RELEASE)
	if (CRenderer::CV_r_ParticlesDebug)
	{
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_DEBUG0];
		if (rRP.m_pCurObject && CRenderer::CV_r_ParticlesDebug == 2)
		{
			rRP.m_pCurObject->m_RState &= ~OS_TRANSPARENT;
			rRP.m_pCurObject->m_RState |= OS_ADD_BLEND;
		}
	}
#endif

	if (CRenderer::CV_r_wireframe && rd->m_nGraphicsPipeline == 0 && rRP.m_pCurObject)
		rRP.m_pCurObject->m_RState &= ~OS_TRANSPARENT;

	assert(rRP.m_pCurObject);
	PREFAST_ASSUME(rRP.m_pCurObject);

	////////////////////////////////////////////////////////////////////////////////////
	IF ((rRP.m_lightVolumeBuffer.HasVolumes()) & ((rRP.m_pCurObject->m_ObjFlags & FOB_LIGHTVOLUME) > 0), 1)
	{
		SRenderObjData* pOD = rRP.m_pCurObject->GetObjData();
#ifndef _RELEASE
		const uint numVolumes = rRP.m_lightVolumeBuffer.GetNumVolumes();
		if (((int32)pOD->m_LightVolumeId - 1) < 0 || ((int32)pOD->m_LightVolumeId - 1) >= numVolumes)
			CRY_ASSERT_MESSAGE(0, "Bad LightVolumeId");
#endif
		rd->RT_SetLightVolumeShaderFlags();
	}

	const bool bIsNearest = (rRP.m_pCurObject->m_ObjFlags & FOB_NEAREST) != 0;
	if (bIsNearest)
	{
		rRP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_NEAREST];
	}
}

void CREParticle::ComputeVertices(SCameraInfo camInfo, uint64 uRenderFlags)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	m_pVertexCreator->ComputeVertices(camInfo, this, uRenderFlags, gRenDev->m_FillRateManager.GetMaxPixels());
}

//////////////////////////////////////////////////////////////////////////
//
// CRenderer particle functions implementation.
//

void CRenderer::EF_AddParticle(CREParticle* pParticle, SShaderItem& shaderItem, CRenderObject* pRO, const SRenderingPassInfo& passInfo)
{
	if (pRO)
	{
		uint32 nBatchFlags;
		int nList;
		int nThreadID = m_RP.m_nFillThreadID;
		if (!EF_GetParticleListAndBatchFlags(nBatchFlags, nList, shaderItem, pRO, passInfo))
			return;

		passInfo.GetRenderView()->AddRenderItem(pParticle, pRO, shaderItem, nList, nBatchFlags, passInfo.GetRendItemSorter(), passInfo.IsShadowPass(), passInfo.IsAuxWindow());
	}
}

void
CRenderer::EF_RemoveParticlesFromScene()
{
	m_FillRateManager.ComputeMaxPixels();
}

void
CRenderer::SyncComputeVerticesJobs()
{
	gEnv->pJobManager->WaitForJob(m_ComputeVerticesJobState);
}

void CRenderer::EF_AddMultipleParticlesToScene(const SAddParticlesToSceneJob* jobs, size_t numJobs, const SRenderingPassInfo& passInfo) PREFAST_SUPPRESS_WARNING(6262)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);
	ASSERT_IS_MAIN_THREAD(m_pRT)

	// update fill thread id for particle jobs
	const CCamera& camera = passInfo.GetCamera();
	int threadList = passInfo.ThreadID();

	// skip particle rendering in rare cases (like after a resolution change)
	if (!m_RP.m_particleBuffer.IsValid())
		return;

	// if we have jobs, set our sync variables to running before starting the jobs
	if (numJobs)
	{
		PrepareParticleRenderObjects(Array<const SAddParticlesToSceneJob>(jobs, numJobs), 0, passInfo);
	}
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::PrepareParticleRenderObjects(Array<const SAddParticlesToSceneJob> aJobs, int nREStart, SRenderingPassInfo passInfoOriginal) PREFAST_SUPPRESS_WARNING(6262)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_PARTICLE);

	SRenderingPassInfo passInfo(passInfoOriginal);

	// == create list of non empty container to submit to the renderer == //
	threadID threadList = passInfo.ThreadID();
	const uint allocId = gRenDev->m_RP.m_particleBuffer.GetAllocId();

	// make sure the GPU doesn't use the VB/IB Buffer we are going to fill anymore
	WaitForParticleBuffer();

	// == now create the render elements and start processing those == //
	const bool useComputeVerticesJob = passInfo.IsGeneralPass();
	if (useComputeVerticesJob)
	{
		m_ComputeVerticesJobState.SetRunning();
	}

	SCameraInfo camInfo(passInfo);

	for (auto& job : aJobs)
	{
		// Generate the RenderItem for this particle container
		int nList;
		uint32 nBatchFlags;
		SShaderItem shaderItem = *job.pShaderItem;
		CRenderObject* pRenderObject = job.pRenderObject;

		const bool isRefractive = shaderItem.m_pShader && (((CShader*)shaderItem.m_pShader)->m_Flags & EF_REFRACTIVE);
		const bool refractionEnabled = CV_r_Refraction && CV_r_ParticlesRefraction;
		if (isRefractive && !refractionEnabled)
			continue;

		size_t ij = &job - aJobs.data();
		CREParticle* pRE = static_cast<CREParticle*>(pRenderObject->m_pRE);

		// generate the RenderItem entries for this Particle Element
		assert(pRenderObject->m_bPermanent);
		EF_GetParticleListAndBatchFlags(
			nBatchFlags, nList, shaderItem,
			pRenderObject, passInfo);
		if (!pRE->AddedToView())
		{
			passInfo.GetRenderView()->AddRenderItem(
				pRE, pRenderObject, shaderItem, nList, nBatchFlags,
				passInfo.GetRendItemSorter(), passInfo.IsShadowPass(),
				passInfo.IsAuxWindow());

			pRE->SetAddedToView();
		}

		pRE->Reset(job.pPVC, threadList, allocId);
		if (job.nCustomTexId > 0)
		{
			pRE->m_CustomTexBind[0] = job.nCustomTexId;
		}
		else
		{
			if (passInfo.IsAuxWindow())
				pRE->m_CustomTexBind[0] = CTexture::s_ptexDefaultProbeCM->GetID();
			else
				pRE->m_CustomTexBind[0] = CTexture::s_ptexBlackCM->GetID();
		}

		if (useComputeVerticesJob)
		{
			// Start new job to compute the vertices
			TComputeVerticesJob cvjob(camInfo, pRenderObject->m_ObjFlags);
			cvjob.SetClassInstance(pRE);
			cvjob.SetPriorityLevel(JobManager::eLowPriority);
			cvjob.RegisterJobState(&m_ComputeVerticesJobState);
			cvjob.Run();
		}
		else
		{
			// Perform it in same thread.
			pRE->ComputeVertices(camInfo, pRenderObject->m_ObjFlags);
		}

		passInfo.GetRendItemSorter().IncreaseParticleCounter();
	}

	if (useComputeVerticesJob)
	{
		m_ComputeVerticesJobState.SetStopped();
	}
}

///////////////////////////////////////////////////////////////////////////////
bool CRenderer::EF_GetParticleListAndBatchFlags(uint32& nBatchFlags, int& nList, SShaderItem& shaderItem, CRenderObject* pRO, const SRenderingPassInfo& passInfo)
{
	nBatchFlags = FB_GENERAL;
	nBatchFlags |= (pRO->m_ObjFlags & FOB_AFTER_WATER) ? 0 : FB_BELOW_WATER;

	const uint8 uHalfResBlend = CV_r_ParticlesHalfResBlendMode ? OS_ADD_BLEND : OS_ALPHA_BLEND;
	bool bHalfRes = (CV_r_ParticlesHalfRes + (pRO->m_ParticleObjFlags & CREParticle::ePOF_HALF_RES)) >= 2   // cvar allows or forces half-res
	                && pRO->m_RState & uHalfResBlend
	                && pRO->m_ObjFlags & FOB_AFTER_WATER;

	const bool bVolumeFog = (pRO->m_ParticleObjFlags & CREParticle::ePOF_VOLUME_FOG) != 0;
	const bool bPulledVertices = (pRO->m_ParticleObjFlags & CREParticle::ePOF_USE_VERTEX_PULL_MODEL) != 0;
	const bool bUseTessShader = !bVolumeFog && (pRO->m_ObjFlags & FOB_ALLOW_TESSELLATION) != 0;

	// Needs to match the technique numbers in the particle shader
	const int TECHNIQUE_TESS = 0;
	const int TECHNIQUE_NO_TESS = 1;
	const int TECHNIQUE_PULLED = 2;
	const int TECHNIQUE_GPU = 3;
	const int TECHNIQUE_VOL_FOG = 4;

	// Adjust shader and flags.
	if (bUseTessShader)
	{
		shaderItem.m_nTechnique = TECHNIQUE_TESS;
		pRO->m_ObjFlags &= ~FOB_OCTAGONAL;
	}
	else if (bVolumeFog)
	{
		// changed to non-tessellation technique.
		shaderItem.m_nTechnique = TECHNIQUE_VOL_FOG;
	}
	else if (bPulledVertices)
	{
		shaderItem.m_nTechnique = TECHNIQUE_PULLED;
	}
	else
	{
		shaderItem.m_nTechnique = TECHNIQUE_NO_TESS;
	}

	SShaderTechnique* pTech = shaderItem.GetTechnique();
	assert(!pTech || (bUseTessShader && pTech->m_Flags & FHF_USE_HULL_SHADER) || !bUseTessShader);

	// Disable vertex instancing on unsupported hardware, or from cvar.
	const bool useTessellation = (pRO->m_ObjFlags & FOB_ALLOW_TESSELLATION) != 0;
#if CRY_PLATFORM_ORBIS
	const bool canUsePointSprites = useTessellation || bPulledVertices;
#else
	const bool canUsePointSprites = useTessellation || CV_r_ParticlesInstanceVertices;
#endif
	if (!canUsePointSprites)
		pRO->m_ObjFlags &= ~FOB_POINT_SPRITE;

	if (shaderItem.m_pShader && (((CShader*)shaderItem.m_pShader)->m_Flags & EF_REFRACTIVE))
	{
		nBatchFlags |= FB_TRANSPARENT;
		if (CRenderer::CV_r_RefractionPartialResolves)
			pRO->m_ObjFlags |= FOB_REQUIRES_RESOLVE;

		bHalfRes = false;
	}

	if (pTech)
	{
		int nThreadID = passInfo.ThreadID();
		SRenderObjData* pOD = pRO->GetObjData();
		if (pTech->m_nTechnique[TTYPE_CUSTOMRENDERPASS] > 0)
		{
			if (m_nThermalVisionMode && pOD->m_nVisionParams)
			{
				nBatchFlags |= FB_CUSTOM_RENDER;
				bHalfRes = false;
			}
		}

		pRO->m_ObjFlags |= (pOD && pOD->m_LightVolumeId) ? FOB_LIGHTVOLUME : 0;
	}

	if (bHalfRes)
		nList = EFSLIST_HALFRES_PARTICLES;
	else if (bVolumeFog)
		nList = EFSLIST_FOG_VOLUME;
	else if (pRO->m_RState & OS_TRANSPARENT)
		nList = EFSLIST_TRANSP;
	else
		nList = EFSLIST_GENERAL;

	return true;
}
