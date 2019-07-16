// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Renderer/SFConfig.h"
#include "Renderer/SFTexture.h"
#include <XRenderD3D9/GraphicsPipeline/Common/PrimitiveRenderPass.h>
#include <list>

namespace Scaleform {
namespace Render {

class CSFRenderCache : public NewOverrideBase<StatRender_RenderPipeline_Mem>, public RefCountVImpl
{
public:
	struct SPSOCacheEntry
	{
		uint32             technique;
		int32              renderState;
		uint32             stencilState;
		SamplerStateHandle samplers[CSFTextureManager::maxTextureSlots];

		bool operator==(const SPSOCacheEntry& pso) const
		{
			if (technique == pso.technique && renderState == pso.renderState && stencilState == pso.stencilState)
			{
				for (uint32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
				{
					if (samplers[i] != pso.samplers[i])
					{
						return false;
					}
				}
				return true;
			}
			return false;
		}
	};

	struct SPSOHash
	{
		uint64 operator()(const SPSOCacheEntry& pso) const
		{
			uint64 hash = pso.technique | uint64(pso.renderState ^ pso.stencilState) << 32;
			for (uint32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
			{
				hash ^= uint64(pso.samplers[i].value) << (16 * i);
			}
			return hash;
		}
	};

	struct SResourceSetCacheEntry
	{
		CTexture*          textures[CSFTextureManager::maxTextureSlots];
		SamplerStateHandle samplers[CSFTextureManager::maxTextureSlots];
		CSFRenderCache*    pCache;

		bool operator==(const SResourceSetCacheEntry& rsce) const
		{
			for (uint32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
			{
				if (textures[i] != rsce.textures[i])
				{
					return false;
				}
			}
			for (uint32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
			{
				if (samplers[i] != rsce.samplers[i])
				{
					return false;
				}
			}
			return pCache == rsce.pCache;
		}
	};

	struct SResourceSetHash
	{
		uint64 operator()(const SResourceSetCacheEntry& rsce) const
		{
			uint64 hash = 0;
			for (uint32 i = 0; i < CSFTextureManager::maxTextureSlots; ++i)
			{
				// Lets not count samplers into hash - the same texture is usually using the same sampler, so the sampler will be mostly a reduntant information.
				// Texture IDs should be in range of uint16 so let's spread all 4 IDs across uint64 hash.
				hash |= uint64(rsce.textures[i]->GetID()) << (16 * i);
			}
			return hash;
		}
	};

	virtual ~CSFRenderCache() override;

	static bool               OnTextureInvalidated(void* pListener, SResourceBindPoint bindPoint, UResourceReference resource, uint32 invalidationFlags);
	CDeviceResourceSetPtr     GetResourceSet(const SResourceSetCacheEntry& resourceSetEntry);
	CDeviceGraphicsPSOPtr     GetPSO(const SPSOCacheEntry& resourceSetEntry, const CPrimitiveRenderPass& targetPass, InputLayoutHandle vertexLayout, CDeviceResourceLayoutPtr& pResourceLayout);
	uint32                    GetTechnique(int32 shaderType);
	CConstantBuffer*          GetConstantBuffer();
	SCompiledRenderPrimitive& GetCompiledPrimitive();
	CPrimitiveRenderPass&     GetRenderPass();
	void                      Clear();

private:
	std::unordered_map<SPSOCacheEntry, std::pair<CDeviceGraphicsPSOPtr, CDeviceResourceLayoutPtr>, SPSOHash> m_psoCache;
	std::unordered_map<SResourceSetCacheEntry, CDeviceResourceSetPtr, SResourceSetHash>                      m_resourceSetCache;
	std::unordered_map<int32, uint32>           m_techniqueCache;
	std::forward_list<CConstantBufferPtr>       m_cachedConstantBuffers;
	std::forward_list<CConstantBufferPtr>       m_usedConstantBuffers;
	std::forward_list<SCompiledRenderPrimitive> m_cachedCompiledPrimitives;
	std::forward_list<SCompiledRenderPrimitive> m_usedCompiledPrimitives;
	std::forward_list<CPrimitiveRenderPass>     m_cachedRenderPasses;
	std::forward_list<CPrimitiveRenderPass>     m_usedRenderPasses;
};

} // ~Render namespace
} // ~Scaleform namespace
