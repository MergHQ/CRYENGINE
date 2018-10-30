// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "NavMeshQueryManager.h"
#include "NavMeshQuery.h"

namespace MNM
{
	//====================================================================
	// CNavMeshQueryManager
	//====================================================================

	const char* s_szFilterByCallerCvarName = "ai_debugDrawNavigationQueryListFilterByCaller";
	const char* s_szFilterByCallerCvarNameDefaultValue = "None";

	CNavMeshQueryManager::CNavMeshQueryManager()
		: m_queryIdCount(0)
	{
		RegisterCVars();
	}

	CNavMeshQueryManager::~CNavMeshQueryManager()
	{
		UnregisterCVars();
	}

	void CNavMeshQueryManager::Clear()
	{
		m_queries.clear();

#ifdef NAV_MESH_QUERY_DEBUG
		m_queriesDebug.clear();
#endif // NAV_MESH_QUERY_DEBUG
	}

	INavMeshQueryPtr CNavMeshQueryManager::CreateQuery(const INavMeshQuery::SNavMeshQueryConfig& queryConf)
	{
		CNavMeshQueryPtr pQuery = std::shared_ptr<CNavMeshQuery>(
			new CNavMeshQuery(m_queryIdCount++, queryConf),
			[&](CNavMeshQuery* pNavMeshQuery)
		{ 
			RemoveQuery(pNavMeshQuery);
			pNavMeshQuery->~CNavMeshQuery();
		});
		m_queries.insert(pQuery.get());

#ifdef NAV_MESH_QUERY_DEBUG
		m_queriesDebug.insert(pQuery.get());
#endif // NAV_MESH_QUERY_DEBUG

		return pQuery;
	}

#ifdef NAV_MESH_QUERY_DEBUG
	const INavMeshQueryManager::INavMeshQueriesPtrSet& CNavMeshQueryManager::GetQueriesDebug() const
	{
		return m_queriesDebug;
	}

	void CNavMeshQueryManager::DebugDrawQueriesList() const
	{
		if (!gAIEnv.CVars.DebugDrawNavigationQueriesList)
		{
			return;
		}

		// Setup
		const ColorF titleColor = Col_Red;
		const ColorF queryColor = Col_White;
		const ColorF batchColor = Col_Grey;

		const float spacingQuery = 20.0f;
		const float spacingBatch = 15.0f;

		const float headerSize = 1.75f;
		const float textQuerySize = 1.5f;
		const float textBatchSize = 1.25f;

		IRenderer* pRenderer = gEnv->pRenderer;
		IRenderAuxGeom* pAux = pRenderer->GetIRenderAuxGeom();

		SAuxGeomRenderFlags flags = pAux->GetRenderFlags();
		SAuxGeomRenderFlags renderFlagsRestore = flags;

		flags.SetMode2D3DFlag(e_Mode2D);
		flags.SetDrawInFrontMode(e_DrawInFrontOn);
		flags.SetDepthTestFlag(e_DepthTestOff);
		pAux->SetRenderFlags(flags);
		pAux->SetRenderFlags(renderFlagsRestore);

		const int screenX = 0;
		const int screenY = 0;
		const int screenWidth = pAux->GetCamera().GetViewSurfaceX();
		const int screenHeight = pAux->GetCamera().GetViewSurfaceZ();
		const Vec2 viewportOrigin(static_cast <float> (screenX), static_cast <float> (screenY));
		const Vec2 viewportSize(static_cast <float> (screenWidth), static_cast <float> (screenHeight));

		// Draw Title (with/out filter)
		const ICVar* filterCvar = gEnv->pConsole->GetCVar(s_szFilterByCallerCvarName);
		const bool cvarEnabled = filterCvar && filterCvar->GetString() != nullptr && filterCvar->GetString()[0] != 0 && stricmp(filterCvar->GetString(), s_szFilterByCallerCvarNameDefaultValue) != 0;
		const char* szFilter = cvarEnabled ? filterCvar->GetString() : s_szFilterByCallerCvarNameDefaultValue;

		Vec2 screenPosition = Vec2(10, 10);
		IRenderAuxText::Draw2dLabel(screenPosition.x, screenPosition.y, headerSize, titleColor, false, "Navigation Queries List (filter = %s)", szFilter);

		// Draw queries
		for (const auto& activeQuery : m_queriesDebug)
		{
			if (!cvarEnabled || CryStringUtils::stristr(activeQuery->GetQueryConfig().szCallerName, szFilter) != nullptr)
			{
				screenPosition.y += spacingQuery;
				const INavMeshQueryDebug& queryDebug = activeQuery->GetQueryDebug();
				const INavMeshQueryDebug::SQueryDebugData& debugData = queryDebug.GetDebugData();

				IRenderAuxText::Draw2dLabel(screenPosition.x, screenPosition.y, textQuerySize, queryColor, false, "ID: %u \tBATCHES: %zu \tTRIANGLES: %zu \tTIME(RUNNING): %f ms \tTIME(ALIVE): %f ms \tCALLER: %s", activeQuery->GetId(), debugData.batchHistory.size(), debugData.trianglesCount, debugData.elapsedTimeRunningInMs, debugData.elapsedTimeTotalInMs, activeQuery->GetQueryConfig().szCallerName);

				// Draw queries batches
				for (const INavMeshQueryDebug::SBatchData& batchData : debugData.batchHistory)
				{
					screenPosition.y += spacingBatch;
					IRenderAuxText::Draw2dLabel(screenPosition.x, screenPosition.y, textBatchSize, batchColor, false, "\t- N: %u \tSIZE: %zu \tTRIANGLES: %zu \tTIME (RUNNING): %f ms", batchData.batchNumber, batchData.batchSize, batchData.triangleDataArray.size(), batchData.elapsedTimeInMs);
				}
			}
		}
	}

#endif // NAV_MESH_QUERY_DEBUG

	void CNavMeshQueryManager::RegisterCVars() const
	{
		ICVar* filterCvar = REGISTER_STRING(s_szFilterByCallerCvarName, nullptr, VF_NULL, 
			"Navigation Queries list with only display those queries that contain the given filter string \n"
			"By default value is set to None (no filter is applied).");
		filterCvar->SetFromString(s_szFilterByCallerCvarNameDefaultValue);
	}

	void CNavMeshQueryManager::UnregisterCVars() const
	{
		IConsole* pConsole = gEnv->pConsole;
		pConsole->UnregisterVariable(s_szFilterByCallerCvarName, true);
	}

	// Automatically called from the query when it's destroyed
	void CNavMeshQueryManager::RemoveQuery(CNavMeshQuery* pQuery)
	{
		m_queries.erase(pQuery);

#ifdef NAV_MESH_QUERY_DEBUG
		m_queriesDebug.erase(pQuery);
#endif // NAV_MESH_QUERY_DEBUG
	}
}