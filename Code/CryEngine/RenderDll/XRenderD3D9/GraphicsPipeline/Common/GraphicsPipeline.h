// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GraphicsPipelineStage.h"
#include "GraphicsPipelineStateSet.h"
#include "UtilityPasses.h"

class CGraphicsPipelineStage;

class CGraphicsPipeline
{
public:
	CGraphicsPipeline()
		: m_numUtilityPasses(0)
	{
		m_pipelineStages.fill(nullptr);
		m_utilityPasses.fill(nullptr);
	}

	virtual ~CGraphicsPipeline()
	{
		for (auto pStage : m_pipelineStages)
		{
			SAFE_DELETE(pStage);
		}
		for (auto pPass : m_utilityPasses)
		{
			SAFE_DELETE(pPass);
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

	template<class T> T* CreateStaticUtilityPass()
	{
		assert(m_numUtilityPasses < m_utilityPasses.size());
		if (m_numUtilityPasses < m_utilityPasses.size())
		{
			T* pUtilityPass = new T();
			m_utilityPasses[m_numUtilityPasses++] = pUtilityPass;
			return pUtilityPass;
		}
		return nullptr;
	}

protected:
	std::array<CGraphicsPipelineStage*, 32> m_pipelineStages;
	std::array<IUtilityRenderPass*, 32>     m_utilityPasses;
	uint32 m_numUtilityPasses;
};
