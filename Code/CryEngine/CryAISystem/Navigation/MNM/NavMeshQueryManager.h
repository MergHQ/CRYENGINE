// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQueryManager.h>

namespace MNM
{
	class CNavMeshQuery;

	class CNavMeshQueryManager final : public INavMeshQueryManager
	{
		typedef std::unordered_set<INavMeshQuery*> INavMeshQueriesPtrSet;

#ifdef NAV_MESH_QUERY_DEBUG


#endif // NAV_MESH_QUERY_DEBUG
	public:
		CNavMeshQueryManager();
		~CNavMeshQueryManager();

		virtual void                                               Clear() override;
		virtual INavMeshQueryPtr                                   CreateQueryBatch(const INavMeshQuery::SNavMeshQueryConfigBatch& queryConf) override;
		virtual INavMeshQuery::EQueryStatus                        RunInstantQuery(const INavMeshQuery::SNavMeshQueryConfigInstant& queryConf, INavMeshQueryProcessing& queryProcessing, const size_t processingBatchSize = 1024) override;

#ifdef NAV_MESH_QUERY_DEBUG
		virtual void                                               DebugDrawQueriesList() const override;
		virtual const INavMeshQueryManager::INavMeshQueriesPtrSet& GetQueriesDebug() const override;
#endif // NAV_MESH_QUERY_DEBUG

	private:
		void                                                       RegisterCVars() const;
		void                                                       UnregisterCVars() const;

		NavMeshQueryId                                             m_queryIdCount;

#ifdef NAV_MESH_QUERY_DEBUG
		INavMeshQueryManager::INavMeshQueriesPtrSet                m_queriesDebug;
#endif // NAV_MESH_QUERY_DEBUG
	};
}