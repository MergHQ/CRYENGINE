// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "RenderView.h"

#include "DriverD3D.h"

#include "GraphicsPipeline/ShadowMap.h"
#include "CompiledRenderObject.h"

//////////////////////////////////////////////////////////////////////////
// Multi-threaded draw-command recording /////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <CryThreading/IJobManager.h>
#include <CryThreading/IJobManager_JobDelegator.h>

ILINE void UpdateRenderPass(CSceneRenderPass& pass, CDeviceGraphicsCommandListRef commandList, CSceneRenderPass*& pRenderPass)
{
	if (pRenderPass != &pass)
	{
		if (pRenderPass)
		{
			pRenderPass->EndRenderPass(commandList);
		}

		pRenderPass = &pass;

		pass.BeginRenderPass(commandList);
	}
}

ILINE void UpdateNearestState(const CSceneRenderPass& pass, CDeviceGraphicsCommandListRef commandList, bool bNearestRenderingRequired, bool& bRenderNearestState)
{
	if (bRenderNearestState != bNearestRenderingRequired)
	{
		bRenderNearestState = bNearestRenderingRequired;

		commandList.SetViewports(1, &pass.GetViewport(bNearestRenderingRequired));
	}
}

#if !CRY_PLATFORM_64BIT
	#define _mm_stream_si64(a, b) (* a = b)
	#define _mm_prefetch(a, b)
#endif

CCompiledRenderObject** FilterCompiledRenderItems(
  const SGraphicsPipelinePassContext* passContext,
  CRenderView::RenderItems* renderItems,
  CCompiledRenderObject** renderList,
  CSceneRenderPass** passList,
  int startRenderItem,
  int endRenderItem
  )
{
	FUNCTION_PROFILER_RENDERER

	const bool bAllowRenderNearest = CRenderer::CV_r_nodrawnear == 0;
	if (!bAllowRenderNearest && passContext->renderNearest)
		return renderList;

	// NOTE: doesn't load-balance well when the conditions for the draw mask lots of draws
	for (int32 i = startRenderItem; i < endRenderItem; ++i)
	{
		SRendItem& ri = (*renderItems)[i];
		if (!(ri.nBatchFlags & passContext->batchFilter))
			continue;

		CCompiledRenderObject* pCompiledObject = ri.pCompiledObject;
		if (pCompiledObject)
		{
			//	if (CRenderer::CV_r_UpdateInstances)
			//		ri.pCompiledObject->CompileInstancingData(ri.pObj, true);

			if (!pCompiledObject->DrawVerification(*passContext))
				continue;

			_mm_stream_si64((int64*)renderList, (int64)pCompiledObject);
			_mm_stream_si64((int64*)passList, (int64)passContext->pSceneRenderPass);
			renderList++;
			passList++;

			ri.nBatchFlags |= FB_COMPILED_OBJECT;
		}
	}

	return renderList;
}

void DrawCompiledRenderItemsToCommandList(
  CCompiledRenderObject** renderList,
  CSceneRenderPass** passList,
  CDeviceGraphicsCommandList* commandList,
  int startRenderItem,
  int endRenderItem
  )
{
#ifndef _RELEASE
	char sItems[32];
	cry_sprintf(sItems, "num:%d", endRenderItem - startRenderItem);
	CRY_PROFILE_FUNCTION_ARG(PROFILE_RENDERER, sItems)
#endif

	bool bRenderNearestState = false;
	CSceneRenderPass* pRenderPass = nullptr;

	// NOTE: does load-balance very well
	int32 i = startRenderItem;
	do
	{
		_mm_prefetch(((char*)passList + i + 4), _MM_HINT_NTA);
		_mm_prefetch(((char*)renderList + i + 4), _MM_HINT_NTA);

		CSceneRenderPass* rPss = passList[i];
		CCompiledRenderObject* rObj = renderList[i];

		UpdateRenderPass(*rPss, *commandList, pRenderPass);
		UpdateNearestState(*rPss, *commandList, rObj->m_bRenderNearest, bRenderNearestState);

		rObj->DrawToCommandList(*commandList, rObj->m_pso[rPss->GetStageID()][rPss->GetPassID()], (rPss->GetStageID() == eStage_ShadowMap) ? 1 : 0);
	}
	while (++i < endRenderItem);
}

void DrawCompiledRenderItemsToCommandList(
  const SGraphicsPipelinePassContext* passContext,
  CRenderView::RenderItems* renderItems,
  CDeviceGraphicsCommandList* commandList,
  int startRenderItem,
  int endRenderItem
  )
{
	FUNCTION_PROFILER_RENDERER

	const bool bAllowRenderNearest = CRenderer::CV_r_nodrawnear == 0;
	if (!bAllowRenderNearest && passContext->renderNearest)
		return;

	passContext->pSceneRenderPass->EnableNearestViewport(passContext->renderNearest);
	passContext->pSceneRenderPass->BeginRenderPass(*commandList);

	// NOTE: doesn't load-balance well when the conditions for the draw mask lots of draws
	for (int32 i = startRenderItem; i < endRenderItem; ++i)
	{
		SRendItem& ri = (*renderItems)[i];
		if (!(ri.nBatchFlags & passContext->batchFilter))
			continue;

		if (ri.pCompiledObject)
		{
			if (!ri.pCompiledObject->DrawVerification(*passContext))
				continue;

			ri.pCompiledObject->DrawToCommandList(*commandList, ri.pCompiledObject->m_pso[passContext->stageID][passContext->passID], (passContext->stageID == eStage_ShadowMap) ? 1 : 0);
			ri.nBatchFlags |= FB_COMPILED_OBJECT;
		}
	}

	passContext->pSceneRenderPass->EndRenderPass(*commandList);
}

// NOTE: Job-System can't handle references (copies) and can't use static member functions or __restrict (doesn't execute)
void ObjectDrawCommandRecorderJob(
  CCompiledRenderObject** renderList,
  CSceneRenderPass** passList,
  CDeviceGraphicsCommandList* commandList,
  int startRenderItem,
  int endRenderItem
  )
{
	FUNCTION_PROFILER_RENDERER

	commandList->LockToThread();

	DrawCompiledRenderItemsToCommandList(
	  renderList,
	  passList,
	  commandList,
	  startRenderItem,
	  endRenderItem
	  );

	commandList->Build();
}

// NOTE: Job-System can't handle references (copies) and can't use static member functions or __restrict (doesn't execute)
void DirectDrawCommandRecorderJob(
  SGraphicsPipelinePassContext passContext,
  CRenderView::RenderItems* renderItems,
  CDeviceGraphicsCommandList* commandList,
  int startRenderItem,
  int endRenderItem
  )
{
	FUNCTION_PROFILER_RENDERER

	commandList->LockToThread();

	DrawCompiledRenderItemsToCommandList(
	  &passContext,
	  renderItems,
	  commandList,
	  startRenderItem,
	  endRenderItem
	  );

	commandList->Build();
}

// NOTE: Job-System can't handle references (copies) and can't use static member functions or __restrict (doesn't execute)
void ListDrawCommandRecorderJob(
  SGraphicsPipelinePassContext* passContext,
  CDeviceGraphicsCommandList* commandList,
  int startRenderItem,
  int endRenderItem
  )
{
	FUNCTION_PROFILER_RENDERER

	commandList->LockToThread();

	int cursor = 0;
	do
	{
		int length = passContext->rendItems.Length();

		// bounding-range check (1D bbox)
		if ((startRenderItem <= (cursor + length)) && (cursor < endRenderItem))
		{
			int offset = std::max(startRenderItem, cursor) - cursor;
			int count = std::min(cursor + length, endRenderItem) - std::max(startRenderItem, cursor);

			auto& RESTRICT_REFERENCE renderItems = passContext->pRenderView->GetRenderItems(passContext->renderListId);

			DrawCompiledRenderItemsToCommandList(
			  passContext,
			  &renderItems,
			  commandList,
			  passContext->rendItems.start + offset,
			  passContext->rendItems.start + offset + count
			  );
		}

		cursor += length;
		passContext += 1;
	}
	while (cursor < endRenderItem);

	commandList->Build();
}

// CV_r_multithreadedDrawingCoalesceMode = 0
DECLARE_JOB("DirectDrawCommandRecorder", TDirectDrawCommandRecorder, DirectDrawCommandRecorderJob);
// CV_r_multithreadedDrawingCoalesceMode = 1
DECLARE_JOB("ListDrawCommandRecorder", TListDrawCommandRecorder, ListDrawCommandRecorderJob);
// CV_r_multithreadedDrawingCoalesceMode = 2
DECLARE_JOB("ObjectDrawCommandRecorder", TObjectDrawCommandRecorder, ObjectDrawCommandRecorderJob);

void CRenderItemDrawer::DrawCompiledRenderItems(const SGraphicsPipelinePassContext& passContext) const
{
	if (passContext.rendItems.IsEmpty())
		return;
	if (CRenderer::CV_r_NoDraw == 2) // Completely skip filling of the command list.
		return;

	PROFILE_FRAME(DrawCompiledRenderItems);

	// Should take items from passContext and be view dependent.
	CRenderView* pRenderView = passContext.pRenderView;
	auto& RESTRICT_REFERENCE renderItems = pRenderView->GetRenderItems(passContext.renderListId);

	{
		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

		passContext.pSceneRenderPass->PrepareRenderPassForUse(commandList);
	}

#ifdef CRY_USE_DX12
	if ((gcpRendD3D->m_nGraphicsPipeline >= 2) && (CRenderer::CV_r_multithreadedDrawing != 0))
	{
		if (CRenderer::CV_r_multithreadedDrawingCoalesceMode > 0)
		{
			// collect all compiled object pointers into the root of the RenderView hierarchy
			while (pRenderView->GetParentView())
			{
				pRenderView = pRenderView->GetParentView();
			}

			// CV_r_multithreadedDrawingCoalesceMode = 1
			if (CRenderer::CV_r_multithreadedDrawingCoalesceMode == 1)
			{
				pRenderView->GetDrawer().m_CoalesceListsMT.Reserve(1);
				pRenderView->GetDrawer().m_CoalesceListsMT.Expand(passContext);
			}
			// CV_r_multithreadedDrawingCoalesceMode = 2
			else
			{
				pRenderView->GetDrawer().m_CoalesceObjectsMT.Reserve(passContext.rendItems.Length());

				CCompiledRenderObject** rendEnd = pRenderView->GetDrawer().m_CoalesceObjectsMT.rendEnd;
				CSceneRenderPass** passEnd = pRenderView->GetDrawer().m_CoalesceObjectsMT.passEnd;

				auto rendEndNew = FilterCompiledRenderItems(
				  &passContext,
				  &renderItems,
				  rendEnd,
				  passEnd,
				  passContext.rendItems.start,
				  passContext.rendItems.end
				  );

				pRenderView->GetDrawer().m_CoalesceObjectsMT.Expand(rendEndNew);
			}
		}
		// CV_r_multithreadedDrawingCoalesceMode = 0
		else
		{
			uint32 numItems = passContext.rendItems.Length();
			if (numItems > 0)
			{
				int32 numTasksTentative = CRenderer::CV_r_multithreadedDrawing;
				uint32 numTasks = std::min(numItems, uint32(numTasksTentative > 0 ? numTasksTentative : std::max(1U, gEnv->GetJobManager()->GetNumWorkerThreads() - 2U)));
				uint32 numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;

				if (CRenderer::CV_r_multithreadedDrawingMinJobSize > 0)
				{
					if ((numTasks > 1) && (numItemsPerTask < CRenderer::CV_r_multithreadedDrawingMinJobSize))
					{
						numTasks = std::max(1U, numItems / CRenderer::CV_r_multithreadedDrawingMinJobSize);
						numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;
					}
				}

				if ((numTasksTentative = numTasks) > 1)
				{
					std::vector<CDeviceGraphicsCommandListUPtr> pCommandLists = CCryDeviceWrapper::GetObjectFactory().AcquireGraphicsCommandLists(numTasks);
					JobManager::SJobState jobState;

					for (uint32 curTask = 1; curTask < numTasks; ++curTask)
					{
						uint32 taskRIStart = passContext.rendItems.start + ((curTask + 0) * numItemsPerTask);
						uint32 taskRIEnd = passContext.rendItems.start + ((curTask + 1) * numItemsPerTask);

						TDirectDrawCommandRecorder job(passContext, &renderItems, pCommandLists[curTask].get(), taskRIStart, taskRIEnd < passContext.rendItems.end ? taskRIEnd : passContext.rendItems.end);

						job.RegisterJobState(&jobState);
						job.SetPriorityLevel(JobManager::eHighPriority);
						job.Run();
					}

					// Execute first task directly on render thread
					{
						PROFILE_FRAME("ListDrawCommandRecorderJob");

						uint32 taskRIStart = 0;
						uint32 taskRIEnd = numItemsPerTask;

						DirectDrawCommandRecorderJob(passContext, &renderItems, pCommandLists[0].get(), taskRIStart, taskRIEnd < passContext.rendItems.end ? taskRIEnd : passContext.rendItems.end);
					}

					gEnv->pJobManager->WaitForJob(jobState);
					CCryDeviceWrapper::GetObjectFactory().ForfeitGraphicsCommandLists(std::move(pCommandLists));
				}

				if (numTasksTentative <= 1)
				{
					auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

					DrawCompiledRenderItemsToCommandList(
					  &passContext,
					  &renderItems,
					  &commandList,
					  passContext.rendItems.start,
					  passContext.rendItems.end
					  );
				}
			}
		}
	}
	else
#endif // CRY_USE_DX12
	{
		auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

		DrawCompiledRenderItemsToCommandList(
		  &passContext,
		  &renderItems,
		  &commandList,
		  passContext.rendItems.start,
		  passContext.rendItems.end
		  );
	}
}

void CRenderItemDrawer::InitDrawSubmission()
{
#ifdef CRY_USE_DX12
	if ((gcpRendD3D->m_nGraphicsPipeline < 2) || (CRenderer::CV_r_multithreadedDrawing == 0))
	{
		return;
	}

	m_CoalesceListsMT.Init();
	m_CoalesceObjectsMT.Init();
#endif // CRY_USE_DX12
}

void CRenderItemDrawer::JobifyDrawSubmission()
{
#ifdef CRY_USE_DX12
	if ((gcpRendD3D->m_nGraphicsPipeline < 2) || (CRenderer::CV_r_multithreadedDrawing == 0))
	{
		return;
	}

	PROFILE_FRAME(JobifyDrawSubmission);

	// CV_r_multithreadedDrawingCoalesceMode = 1
	{
		SGraphicsPipelinePassContext* rendStart = m_CoalesceListsMT.rendStart;
		SGraphicsPipelinePassContext* rendEnd = m_CoalesceListsMT.rendEnd;
		SGraphicsPipelinePassContext* rendCursor = rendStart;

		uint32 numItems = 0;
		rendCursor = rendStart;
		while (rendCursor != rendEnd)
		{
			numItems += rendCursor->rendItems.Length();
			rendCursor += 1;
		}

		if (numItems > 0)
		{
			int32 numTasksTentative = CRenderer::CV_r_multithreadedDrawing;
			uint32 numTasks = std::min(numItems, uint32(numTasksTentative > 0 ? numTasksTentative : std::max(1U, gEnv->GetJobManager()->GetNumWorkerThreads() - 2U)));
			uint32 numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;

			if (CRenderer::CV_r_multithreadedDrawingMinJobSize > 0)
			{
				if ((numTasks > 1) && (numItemsPerTask < CRenderer::CV_r_multithreadedDrawingMinJobSize))
				{
					numTasks = std::max(1U, numItems / CRenderer::CV_r_multithreadedDrawingMinJobSize);
					numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;
				}
			}

			if (((numTasksTentative = numTasks) > 1) || (CRenderer::CV_r_multithreadedDrawingMinJobSize == 0))
			{
				m_CoalesceListsMT.PrepareJobs(numTasks);

				for (uint32 curTask = 1; curTask < numTasks; ++curTask)
				{
					uint32 taskRIStart = 0 + ((curTask + 0) * numItemsPerTask);
					uint32 taskRIEnd = 0 + ((curTask + 1) * numItemsPerTask);

					TListDrawCommandRecorder job(rendStart, m_CoalesceListsMT.pCommandLists[curTask].get(), taskRIStart, taskRIEnd < numItems ? taskRIEnd : numItems);

					job.RegisterJobState(&m_CoalesceListsMT.jobState);
					job.SetPriorityLevel(JobManager::eHighPriority);
					job.Run();
				}

				// Execute first task directly on render thread
				{
					PROFILE_FRAME("ListDrawCommandRecorderJob");

					uint32 taskRIStart = 0;
					uint32 taskRIEnd = numItemsPerTask;

					ListDrawCommandRecorderJob(rendStart, m_CoalesceListsMT.pCommandLists[0].get(), taskRIStart, taskRIEnd < numItems ? taskRIEnd : numItems);
				}
			}

			if ((numTasksTentative <= 1) && (CRenderer::CV_r_multithreadedDrawingMinJobSize != 0))
			{
				rendCursor = rendStart;
				while (rendCursor != rendEnd)
				{
					const SGraphicsPipelinePassContext& passContext = *rendCursor;

					// Should take items from passContext and be view dependent.
					CRenderView* pRenderView = passContext.pRenderView;
					auto& RESTRICT_REFERENCE renderItems = pRenderView->GetRenderItems(passContext.renderListId);
					auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

					DrawCompiledRenderItemsToCommandList(
					  &passContext,
					  &renderItems,
					  &commandList,
					  passContext.rendItems.start,
					  passContext.rendItems.end
					  );

					rendCursor += 1;
				}
			}
		}
	}

	// CV_r_multithreadedDrawingCoalesceMode = 2
	{
		CCompiledRenderObject** rendStart = m_CoalesceObjectsMT.rendStart;
		CCompiledRenderObject** rendEnd = m_CoalesceObjectsMT.rendEnd;
		CSceneRenderPass** passStart = m_CoalesceObjectsMT.passStart;
		CSceneRenderPass** passEnd = m_CoalesceObjectsMT.passEnd;

		const uint32 numItems = uint32(rendEnd - rendStart);
		if (numItems > 0)
		{
			int32 numTasksTentative = CRenderer::CV_r_multithreadedDrawing;
			uint32 numTasks = std::min(numItems, uint32(numTasksTentative > 0 ? numTasksTentative : std::max(1U, gEnv->GetJobManager()->GetNumWorkerThreads() - 2U)));
			uint32 numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;

			if (CRenderer::CV_r_multithreadedDrawingMinJobSize > 0)
			{
				if ((numTasks > 1) && (numItemsPerTask < CRenderer::CV_r_multithreadedDrawingMinJobSize))
				{
					numTasks = std::max(1U, numItems / CRenderer::CV_r_multithreadedDrawingMinJobSize);
					numItemsPerTask = (numItems + (numTasks - 1)) / numTasks;
				}
			}

			if (((numTasksTentative = numTasks) > 1) || (CRenderer::CV_r_multithreadedDrawingMinJobSize == 0))
			{
				m_CoalesceObjectsMT.PrepareJobs(numTasks);

				for (uint32 curTask = 1; curTask < numTasks; ++curTask)
				{
					uint32 taskRIStart = 0 + ((curTask + 0) * numItemsPerTask);
					uint32 taskRIEnd = 0 + ((curTask + 1) * numItemsPerTask);

					TObjectDrawCommandRecorder job(rendStart, passStart, m_CoalesceObjectsMT.pCommandLists[curTask].get(), taskRIStart, taskRIEnd < numItems ? taskRIEnd : numItems);

					job.RegisterJobState(&m_CoalesceObjectsMT.jobState);
					job.SetPriorityLevel(JobManager::eHighPriority);
					job.Run();
				}

				// Execute first task directly on render thread
				{
					PROFILE_FRAME("ListDrawCommandRecorderJob");

					uint32 taskRIStart = 0;
					uint32 taskRIEnd = numItemsPerTask;

					ObjectDrawCommandRecorderJob(rendStart, passStart, m_CoalesceObjectsMT.pCommandLists[0].get(), taskRIStart, taskRIEnd < numItems ? taskRIEnd : numItems);
				}
			}

			if ((numTasksTentative <= 1) && (CRenderer::CV_r_multithreadedDrawingMinJobSize != 0))
			{
				auto& RESTRICT_REFERENCE commandList = *CCryDeviceWrapper::GetObjectFactory().GetCoreGraphicsCommandList();

				DrawCompiledRenderItemsToCommandList(
				  rendStart,
				  passStart,
				  &commandList,
				  0,
				  numItems
				  );
			}
		}
	}
#endif // CRY_USE_DX12
}

void CRenderItemDrawer::WaitForDrawSubmission()
{
#ifdef CRY_USE_DX12
	if ((gcpRendD3D->m_nGraphicsPipeline < 2) || (CRenderer::CV_r_multithreadedDrawing == 0))
	{
		return;
	}

	CRY_PROFILE_FUNCTION_WAITING(PROFILE_RENDERER)

	m_CoalesceListsMT.WaitForJobs();
	m_CoalesceObjectsMT.WaitForJobs();
#endif // CRY_USE_DX12
}
