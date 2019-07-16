// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "Renderer/SFRenderCache.h"
#include "Renderer/SFShaderDescs.h"

namespace Scaleform {
namespace Render {

CSFRenderCache::~CSFRenderCache()
{
	for (const auto& entry : m_resourceSetCache)
	{
		for (int32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
		{
			if (CRendererResources::s_ptexNoTexture != entry.first.textures[i])
			{
				entry.first.textures[i]->RemoveInvalidateCallbacks(const_cast<SResourceSetCacheEntry*>(&entry.first));
			}
		}
	}
}

bool CSFRenderCache::OnTextureInvalidated(void* pListener, SResourceBindPoint bindPoint, UResourceReference resource, uint32 invalidationFlags)
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	SResourceSetCacheEntry* pResourceSetEntry = static_cast<SResourceSetCacheEntry*>(pListener);
	pResourceSetEntry->pCache->m_resourceSetCache.erase(*pResourceSetEntry);

	return false;
}

CDeviceResourceSetPtr CSFRenderCache::GetResourceSet(const SResourceSetCacheEntry& resourceSetEntry)
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	const auto& it = m_resourceSetCache.find(resourceSetEntry);
	if (it != m_resourceSetCache.end())
	{
		return (it->second);
	}

	CDeviceResourceSetDesc resourcesSetDesc;
	for (int32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
	{
		resourcesSetDesc.SetTexture(i, resourceSetEntry.textures[i], EDefaultResourceViews::Default, EShaderStage_Pixel);
		resourcesSetDesc.SetSampler(i, resourceSetEntry.samplers[i], EShaderStage_Pixel);
	}

	CDeviceResourceSetPtr pResourceSet = GetDeviceObjectFactory().CreateResourceSet(CDeviceResourceSet::EFlags_ForceSetAllState);
	if (pResourceSet->Update(resourcesSetDesc))
	{
		const auto& new_it = m_resourceSetCache.insert({ resourceSetEntry, pResourceSet });
		for (int32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
		{
			if (CRendererResources::s_ptexNoTexture != resourceSetEntry.textures[i])
			{
				resourceSetEntry.textures[i]->AddInvalidateCallback(const_cast<SResourceSetCacheEntry*>(&new_it.first->first), SResourceBindPoint(), OnTextureInvalidated);
			}
		}
	}

	return pResourceSet;
}

CDeviceGraphicsPSOPtr CSFRenderCache::GetPSO(const SPSOCacheEntry& resourceSetEntry, const CPrimitiveRenderPass& targetPass, InputLayoutHandle vertexLayout, CDeviceResourceLayoutPtr& pResourceLayout)
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	auto it = m_psoCache.find(resourceSetEntry);
	if (it != m_psoCache.end())
	{
		pResourceLayout = it->second.second;
		return it->second.first;
	}

	SDeviceResourceLayoutDesc resourceLayoutDesc;
	CDeviceResourceSetDesc resourcesSetDesc;

	for (int32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
	{
		resourcesSetDesc.SetTexture(i, nullptr, EDefaultResourceViews::Default, EShaderStage_Pixel);
		resourcesSetDesc.SetSampler(i, resourceSetEntry.samplers[i], EShaderStage_Pixel);
	}

	resourceLayoutDesc.SetResourceSet(0, resourcesSetDesc);
	resourceLayoutDesc.SetConstantBuffer(1, eConstantBufferShaderSlot_PerDraw, EShaderStage_Vertex);
	resourceLayoutDesc.SetConstantBuffer(2, eConstantBufferShaderSlot_PerDraw, EShaderStage_Pixel);
	pResourceLayout = GetDeviceObjectFactory().CreateResourceLayout(resourceLayoutDesc);

	CDeviceGraphicsPSODesc psoDesc;
	psoDesc.m_pResourceLayout = pResourceLayout;
	psoDesc.m_pShader = CShaderMan::s_ShaderScaleForm;
	psoDesc.m_technique = resourceSetEntry.technique;
	psoDesc.m_ShaderFlags_RT = 0;
	psoDesc.m_ShaderFlags_MD = 0;
	psoDesc.m_ShaderFlags_MDV = MDV_NONE;
	psoDesc.m_PrimitiveType = eptTriangleList;
	psoDesc.m_VertexFormat = vertexLayout;
	psoDesc.m_RenderState = resourceSetEntry.renderState;
	psoDesc.m_StencilState = resourceSetEntry.stencilState;
	psoDesc.m_StencilReadMask = 0xFF;
	psoDesc.m_StencilWriteMask = 0xFF;
	psoDesc.m_CullMode = eCULL_None;
	psoDesc.m_bDepthClip = true;
	psoDesc.m_pRenderPass = targetPass.GetRenderPass();
	CDeviceGraphicsPSOPtr pPipelineState = GetDeviceObjectFactory().CreateGraphicsPSO(psoDesc);
	m_psoCache[resourceSetEntry] = { pPipelineState, pResourceLayout };
	return pPipelineState;
}

uint32 CSFRenderCache::GetTechnique(int32 shaderType)
{
	auto it = m_techniqueCache.find(shaderType);
	if (it != m_techniqueCache.end())
	{
		return it->second;
	}

	const char* name = D3D1x::ShaderDesc::GetShaderName(static_cast<D3D1x::ShaderDesc::ShaderType>(shaderType));
	return m_techniqueCache[shaderType] = CCryNameTSCRC(name).get();
}

SCompiledRenderPrimitive& CSFRenderCache::GetCompiledPrimitive()
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	if (m_cachedCompiledPrimitives.begin() == m_cachedCompiledPrimitives.end())
	{
		m_usedCompiledPrimitives.emplace_front();

		SCompiledRenderPrimitive& compiledPrimitive = *m_usedCompiledPrimitives.begin();
		compiledPrimitive.m_inlineConstantBuffers[0].shaderSlot = eConstantBufferShaderSlot_PerDraw;
		compiledPrimitive.m_inlineConstantBuffers[0].shaderStages = EShaderStage_Vertex;
		compiledPrimitive.m_inlineConstantBuffers[1].shaderSlot = eConstantBufferShaderSlot_PerDraw;
		compiledPrimitive.m_inlineConstantBuffers[1].shaderStages = EShaderStage_Pixel;
		return compiledPrimitive;
	}
	else
	{
		m_usedCompiledPrimitives.splice_after(m_usedCompiledPrimitives.before_begin(), m_cachedCompiledPrimitives, m_cachedCompiledPrimitives.before_begin());
		return *m_usedCompiledPrimitives.begin();
	}
}

CPrimitiveRenderPass& CSFRenderCache::GetRenderPass()
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	if (m_cachedRenderPasses.begin() == m_cachedRenderPasses.end())
	{
		m_usedRenderPasses.emplace_front();
		return *m_usedRenderPasses.begin();
	}
	else
	{
		m_usedRenderPasses.splice_after(m_usedRenderPasses.before_begin(), m_cachedRenderPasses, m_cachedRenderPasses.before_begin());

		CPrimitiveRenderPass& renderPass = *m_usedRenderPasses.begin();
		renderPass.SetTargetClearMask(0);
		renderPass.ClearPrimitives();
		return renderPass;
	}
}

CConstantBuffer* CSFRenderCache::GetConstantBuffer()
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());

	if (m_cachedConstantBuffers.begin() == m_cachedConstantBuffers.end())
	{
		const buffer_size_t size = D3D1x::Uniform::SU_VertexSize* sizeof(float);
		m_usedConstantBuffers.emplace_front(((CD3D9Renderer*)gEnv->pRenderer)->m_DevBufMan.CreateConstantBuffer(size));
	}
	else
	{
		m_usedConstantBuffers.splice_after(m_usedConstantBuffers.before_begin(), m_cachedConstantBuffers, m_cachedConstantBuffers.before_begin());
	}
	return *m_usedConstantBuffers.begin();
}

void CSFRenderCache::Clear()
{
	CRY_ASSERT(((CD3D9Renderer*)gEnv->pRenderer)->m_pRT->IsRenderThread());
	m_cachedRenderPasses.splice_after(m_cachedRenderPasses.before_begin(), m_usedRenderPasses);
	m_cachedConstantBuffers.splice_after(m_cachedConstantBuffers.before_begin(), m_usedConstantBuffers);
	m_cachedCompiledPrimitives.splice_after(m_cachedCompiledPrimitives.before_begin(), m_usedCompiledPrimitives);
}

} // ~Render namespace
} // ~Scaleform namespace
