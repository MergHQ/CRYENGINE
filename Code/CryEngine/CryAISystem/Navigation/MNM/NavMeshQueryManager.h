// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQueryManager.h>

namespace MNM
{
	class CNavMeshQuery;

	class CNavMeshQueryManager final : public INavMeshQueryManager
	{
		typedef std::unordered_set<CNavMeshQuery*> CNavMeshQueriesPtrSet;

	public:
		CNavMeshQueryManager();
		~CNavMeshQueryManager();

		virtual void                                               Clear() override;
		virtual INavMeshQueryPtr                                   CreateQuery(const INavMeshQuery::SNavMeshQueryConfig& queryConf) override;

#ifdef NAV_MESH_QUERY_DEBUG
		virtual void                                               DebugDrawQueriesList() const override;
		virtual const INavMeshQueryManager::INavMeshQueriesPtrSet& GetQueriesDebug() const override;
#endif // NAV_MESH_QUERY_DEBUG

	private:
		void                                                       RegisterCVars() const;
		void                                                       UnregisterCVars() const;
		void                                                       RemoveQuery(CNavMeshQuery* pQuery);

		CNavMeshQueriesPtrSet                                      m_queries;
		NavMeshQueryId                                             m_queryIdCount;

#ifdef NAV_MESH_QUERY_DEBUG
		INavMeshQueryManager::INavMeshQueriesPtrSet                m_queriesDebug;
#endif // NAV_MESH_QUERY_DEBUG
	};
}