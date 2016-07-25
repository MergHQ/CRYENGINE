// Copyright 2001-2015 Crytek GmbH. All rights reserved.

#include "StdAfx.h"
#include "ComputeRenderPass.h"
#include "DriverD3D.h"

CComputeRenderPass::CComputeRenderPass(EPassFlags flags)
	: m_flags(flags)
	, m_dirtyMask(eDirty_All)
	, m_pShader(nullptr)
	, m_rtMask(0)
	, m_dispatchSizeX(1)
	, m_dispatchSizeY(1)
	, m_dispatchSizeZ(1)
	, m_currentPsoUpdateCount(0)
	, m_bPendingConstantUpdate(false)
	, m_prevRTMask(0)
{
	m_inputVars[0] = m_inputVars[1] = m_inputVars[2] = m_inputVars[3] = 0;
	m_pResources = CCryDeviceWrapper::GetObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
}

uint CComputeRenderPass::Compile()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (m_dirtyMask & eDirty_Resources)
	{
		m_pResources->Build();

		if (!m_pResources->IsValid())
			return m_dirtyMask;
	}

	if (m_dirtyMask & (eDirty_Technique | eDirty_Resources))
	{
		// check for valid shader reflection first
		if (m_flags & eFlags_ReflectConstantBuffersFromShader)
		{
			if (!m_constantManager.IsShaderReflectionValid())
				return eDirty_All;
		}

		// Resource layout
		int bindSlot = 0;
		SDeviceResourceLayoutDesc resourceLayoutDesc;

		resourceLayoutDesc.SetResourceSet(bindSlot++, m_pResources);
		for (auto& cb : m_constantManager.GetBuffers())
			resourceLayoutDesc.SetConstantBuffer(bindSlot++, cb.shaderSlot, cb.shaderStages);

		m_pResourceLayout = CCryDeviceWrapper::GetObjectFactory().CreateResourceLayout(resourceLayoutDesc);

		if (!m_pResourceLayout)
			return eDirty_All;
	}

	if (m_dirtyMask & eDirty_Technique)
	{
		// Pipeline state
		CDeviceComputePSODesc psoDesc(m_pResourceLayout.get(), m_pShader, m_techniqueName, m_rtMask, 0, 0);
		m_pPipelineState = CCryDeviceWrapper::GetObjectFactory().CreateComputePSO(psoDesc);

		if (!m_pPipelineState || !m_pPipelineState->IsValid())
			return m_dirtyMask;

		m_currentPsoUpdateCount = m_pPipelineState->GetUpdateCount();
	}

	m_dirtyMask = eDirty_None;
	return m_dirtyMask;
}

void CComputeRenderPass::BeginConstantUpdate()
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	m_prevRTMask = rd->m_RP.m_FlagsShader_RT;
	rd->m_RP.m_FlagsShader_RT = m_rtMask;

	if (m_flags & eFlags_ReflectConstantBuffersFromShader)
	{
		m_bPendingConstantUpdate = true;
		m_constantManager.BeginNamedConstantUpdate();
	}
}

void CComputeRenderPass::PrepareResourcesForUse(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage)
{
	CD3D9Renderer* const __restrict rd = gcpRendD3D;

	if (m_bPendingConstantUpdate)
	{
		// Unmap constant buffers and mark as bound
		m_constantManager.EndNamedConstantUpdate();

		rd->m_RP.m_FlagsShader_RT = m_prevRTMask;
		m_bPendingConstantUpdate = false;
	}

	if (m_dirtyMask != eDirty_None)
	{
		m_dirtyMask = Compile();
	}

	if (m_dirtyMask == eDirty_None)
	{
		CDeviceGraphicsCommandInterface* pGraphicsInterface = commandList.GetGraphicsInterface();
		auto& inlineConstantBuffers = m_constantManager.GetBuffers();

		// Prepare resources
		int bindSlot = 0;
		pGraphicsInterface->PrepareResourcesForUse(bindSlot++, m_pResources.get(), srvUsage);

		for (auto& cb : inlineConstantBuffers)
			pGraphicsInterface->PrepareInlineConstantBufferForUse(bindSlot++, cb.pBuffer, cb.shaderSlot, EShaderStage_Compute);
	}
}

void CComputeRenderPass::BeginRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList)
{
	// Note: Function has to be threadsafe since it can be called from several worker threads

#if defined(ENABLE_PROFILING_CODE)
	commandList.BeginProfilingSection();
#endif
}

void CComputeRenderPass::EndRenderPass(CDeviceCommandListRef RESTRICT_REFERENCE commandList)
{
	// Note: Function has to be threadsafe since it can be called from several worker threads

#if defined(ENABLE_PROFILING_CODE)
	m_profilingStats.Merge(commandList.EndProfilingSection());
#endif

	// Nothing to cleanup at the moment
}

void CComputeRenderPass::Dispatch(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage)
{
	if (m_dirtyMask == eDirty_None)
	{
		CDeviceComputeCommandInterface* pComputeInterface = commandList.GetComputeInterface();
		auto& inlineConstantBuffers = m_constantManager.GetBuffers();

		// Draw to command list
		int bindSlot = 0;
		pComputeInterface->SetResourceLayout(m_pResourceLayout.get());
		pComputeInterface->SetPipelineState(m_pPipelineState.get());
		pComputeInterface->SetResources(bindSlot++, m_pResources.get(), EShaderStage_Compute);

		for (auto& cb : inlineConstantBuffers)
			pComputeInterface->SetInlineConstantBuffer(bindSlot++, cb.pBuffer, cb.shaderSlot);

		pComputeInterface->Dispatch(m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ);
	}
}

void CComputeRenderPass::Execute(CDeviceCommandListRef RESTRICT_REFERENCE commandList, ::EShaderStage srvUsage)
{
	BeginRenderPass(commandList);
	Dispatch(commandList, srvUsage);
	EndRenderPass(commandList);
}
