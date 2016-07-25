// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMath/Range.h>

// forward declarations
typedef std::shared_ptr<CGraphicsPipelineStateLocalCache> CGraphicsPipelineStateLocalCachePtr;
class CGraphicsPipelineStage;
class CSceneRenderPass;
class CRenderView;

struct SGraphicsPipelinePassContext
{
	SGraphicsPipelinePassContext()
	{
	}

	SGraphicsPipelinePassContext(CRenderView* renderView, CSceneRenderPass* pSceneRenderPass, EShaderTechniqueID technique, uint32 filter)
		: pSceneRenderPass(pSceneRenderPass)
		, techniqueID(technique)
		, batchFilter(filter)
		, nFrameID(0)
		, renderListId(EFSLIST_INVALID)
		, stageID(0)
		, passID(0)
		, pRenderView(renderView)
		, renderNearest(false)
	{
	}

	SGraphicsPipelinePassContext& operator=(const SGraphicsPipelinePassContext& other)
	{
		memcpy(this, &other, sizeof(*this));
		return *this;
	}

	CSceneRenderPass*  pSceneRenderPass;
	EShaderTechniqueID techniqueID;
	uint32             batchFilter;

	ERenderListID      renderListId;
	threadID           nProcessThreadID;

	uint64             nFrameID;

	// Stage ID of a scene stage (EStandardGraphicsPipelineStage)
	uint32 stageID;
	// Pass ID, in case a stage has several different scene passes
	uint32 passID;

	// rend items
	CRenderView* pRenderView;
	TRange<int>  rendItems;

	bool         renderNearest;
};

class CGraphicsPipelineStage
{
public:
	virtual ~CGraphicsPipelineStage() {}

	// Allocate resources needed by the graphics pipeline stage
	virtual void Init()                            {}
	// Prepare stage before actual rendering starts (called every frame)
	virtual void Prepare(CRenderView* pRenderView) {}

protected:
	// Helpers.
	static CRenderView* RenderView();

protected:
	friend class CGraphicsPipeline;

	uint32 m_stageID;
};
