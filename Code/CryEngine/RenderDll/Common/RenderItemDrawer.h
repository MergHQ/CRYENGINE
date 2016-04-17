// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryThreading/CryThreadSafeRendererContainer.h>
#include <CryThreading/CryThreadSafeWorkerContainer.h>
#include <CryMath/Range.h>

#include "RenderPipeline.h"
#include "../XRenderD3D9/GraphicsPipeline/Common/GraphicsPipelineStage.h"

class CSceneRenderPass;
class CPermanentRenderObject;

class CRenderItemDrawer
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Multi-threaded draw-command recording /////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	// CV_r_multithreadedDrawingCoalesceMode = 1
	struct SRenderListCollection
	{
		void Init()
		{
			rendEnd = rendStart;
		}

		void Reserve(size_t numAdditionalItems)
		{
			size_t numItems =
			  (rendEnd - rendStart);
			size_t numItemsAfter = numItems +
			                       numAdditionalItems;

			if (rendList.size() < numItemsAfter)
			{
				rendList.resize(numItemsAfter);
				rendStart = &rendList[0];
				rendEnd = rendStart + numItems;
			}
		}

		void Expand(const SGraphicsPipelinePassContext& rendList)
		{
			* rendEnd++ = rendList;
		}

		void WaitForJobs()
		{
			if (pCommandLists.size())
			{
				gEnv->pJobManager->WaitForJob(jobState);
				CCryDeviceWrapper::GetObjectFactory().ForfeitGraphicsCommandLists(std::move(pCommandLists));
			}
		}

		void PrepareJobs(uint32 numTasks)
		{
			WaitForJobs();

			pCommandLists = CCryDeviceWrapper::GetObjectFactory().AcquireGraphicsCommandLists(numTasks);
		}

		std::vector<SGraphicsPipelinePassContext>   rendList;

		SGraphicsPipelinePassContext*               rendStart, * rendEnd;

		std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists;
		JobManager::SJobState                       jobState;
	};

	// CV_r_multithreadedDrawingCoalesceMode = 2
	struct SCompiledObjectCollection
	{
		void Init()
		{
			passEnd = passStart;
			rendEnd = rendStart;
		}

		void Reserve(size_t numAdditionalItems)
		{
			size_t numItems =
			  (rendEnd - rendStart);
			size_t numItemsAfter = numItems +
			                       numAdditionalItems;

			if (rendList.size() < numItemsAfter)
			{
				rendList.resize(numItemsAfter);
				rendStart = &rendList[0];
				rendEnd = rendStart + numItems;
			}

			if (passList.size() < numItemsAfter)
			{
				passList.resize(numItemsAfter);
				passStart = &passList[0];
				passEnd = passStart + numItems;
			}
		}

		void Expand(CCompiledRenderObject** rendEndNew)
		{
			passEnd += rendEndNew - rendEnd;
			rendEnd = rendEndNew;
		}

		void WaitForJobs()
		{
			if (pCommandLists.size())
			{
				gEnv->pJobManager->WaitForJob(jobState);
				CCryDeviceWrapper::GetObjectFactory().ForfeitGraphicsCommandLists(std::move(pCommandLists));
			}
		}

		void PrepareJobs(uint32 numTasks)
		{
			WaitForJobs();

			pCommandLists = CCryDeviceWrapper::GetObjectFactory().AcquireGraphicsCommandLists(numTasks);
		}

		std::vector<CCompiledRenderObject*>         rendList;
		std::vector<CSceneRenderPass*>              passList;

		CCompiledRenderObject**                     rendStart, ** rendEnd;
		CSceneRenderPass**                          passStart, ** passEnd;

		std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists;
		JobManager::SJobState                       jobState;
	};

	// CV_r_multithreadedDrawingCoalesceMode = 1
	SRenderListCollection     m_CoalesceListsMT;
	// CV_r_multithreadedDrawingCoalesceMode = 2
	SCompiledObjectCollection m_CoalesceObjectsMT;

	void DrawCompiledRenderItems(const SGraphicsPipelinePassContext& passContext) const;

	void InitDrawSubmission();
	void JobifyDrawSubmission();
	void WaitForDrawSubmission();
};
