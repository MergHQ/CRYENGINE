// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GraphicsPipelineStage.h"
#include "GraphicsPipelineStateSet.h"
#include "UtilityPasses.h"

class CGraphicsPipelineStage;

class CGraphicsPipeline
{
private:
	struct SUtilityPassCache
	{
		uint32 numUsed = 0;
		std::vector<std::unique_ptr<IUtilityRenderPass>> utilityPasses;
	};

public:
	CGraphicsPipeline()
	{
		m_pipelineStages.fill(nullptr);
	}

	virtual ~CGraphicsPipeline()
	{
		for (auto pStage : m_pipelineStages)
		{
			SAFE_DELETE(pStage);
		}
	}

	// Allocate resources needed by the pipeline and its stages
	virtual void            Init() = 0;
	// Prepare all stages before actual drawing starts
	virtual void            Prepare(CRenderView* pRenderView, EShaderRenderingFlags renderingFlags) = 0;
	// Execute the pipeline and its stages
	virtual void            Execute() = 0;

	CGraphicsPipelineStage* GetStage(uint32 stageID)
	{
		assert(stageID < m_pipelineStages.size());
		assert(m_pipelineStages[stageID] != nullptr);
		return m_pipelineStages[stageID];
	}

protected:
	template<class T> void RegisterStage(T*& pPipelineStage, uint32 stageID)
	{
		assert(stageID < m_pipelineStages.size());
		assert(m_pipelineStages[stageID] == nullptr);
		pPipelineStage = new T();
		pPipelineStage->m_stageID = stageID;
		pPipelineStage->Init();
		m_pipelineStages[stageID] = pPipelineStage;
	}

	// Scene stages contain scene render passes that make use of the PSO cache.
	// Their stage ID is used to index into the PSO cache.
	template<class T, uint32 stageID> void RegisterSceneStage(T*& pPipelineStage)
	{
		static_assert(stageID < MAX_PIPELINE_SCENE_STAGES, "Invalid ID for scene stage");
		RegisterStage<T>(pPipelineStage, stageID);
	}

	template<class T> T* GetOrCreateUtilityPass()
	{
		IUtilityRenderPass::EPassId id = T::GetPassId();
		auto& cache = m_utilityPassCaches[uint32(id)];
		if (cache.numUsed >= cache.utilityPasses.size())
		{
			cache.utilityPasses.emplace_back(new T);
		}
		return static_cast<T*>(cache.utilityPasses[cache.numUsed++].get());
	}

	void ResetUtilityPassCache()
	{
		for (auto& it : m_utilityPassCaches)
		{
			it.numUsed = 0;
		}
	}

protected:
	std::array<CGraphicsPipelineStage*, 32> m_pipelineStages;

private:
	std::array<SUtilityPassCache, uint32(IUtilityRenderPass::EPassId::MaxPassCount)> m_utilityPassCaches;
};
