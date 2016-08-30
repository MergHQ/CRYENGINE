// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Gpu/Particles/GpuParticleManager.h"
#include "GpuParticles.h"

CGpuParticlesStage::CGpuParticlesStage()
	: m_oldFrameIdExecute(-1)
	, m_oldFrameIdPostDraw(-1)
{
}

CGpuParticlesStage::~CGpuParticlesStage()
{
	if (m_pGpuParticleManager)
		m_pGpuParticleManager->CleanupResources();
}

void CGpuParticlesStage::Init()
{
	if (!m_pGpuParticleManager)
		m_pGpuParticleManager = std::unique_ptr<gpu_pfx2::CManager>(new gpu_pfx2::CManager());
}

void CGpuParticlesStage::Prepare(CRenderView* pRenderView)
{
}

void CGpuParticlesStage::Execute(CRenderView* pRenderView)
{
	int newFrameId = gcpRendD3D.GetFrameID(false);
	if (m_oldFrameIdExecute  != newFrameId)
	{
		m_pGpuParticleManager->RenderThreadUpdate();
		m_oldFrameIdExecute = newFrameId;
	}
}


void CGpuParticlesStage::PostDraw(CRenderView* pRenderView)
{
	int newFrameId = gcpRendD3D.GetFrameID(false);
	if (m_oldFrameIdPostDraw != newFrameId)
	{
		m_pGpuParticleManager->RenderThreadPostUpdate();
		m_oldFrameIdPostDraw = newFrameId;
	}
}

gpu_pfx2::IManager* CD3D9Renderer::GetGpuParticleManager()
{
	return GetGraphicsPipeline().GetGpuParticlesStage()->GetGpuParticleManager();
}