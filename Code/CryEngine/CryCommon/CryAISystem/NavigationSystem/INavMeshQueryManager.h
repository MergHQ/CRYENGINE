// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQuery.h>
#include <CryAISystem/NavigationSystem/INavMeshQueryFilter.h>

namespace MNM
{
	//! INavMeshQueryManager is responsible for creating and debugging INavMeshQuery
	struct INavMeshQueryManager
	{
	public:
		typedef std::set<const INavMeshQuery*> INavMeshQueriesPtrSet;

		virtual void                             Clear() = 0;

		virtual INavMeshQueryPtr                 CreateQueryBatch(const INavMeshQuery::SNavMeshQueryConfigBatch& queryConf) = 0;
		virtual INavMeshQuery::EQueryStatus      RunInstantQuery(const INavMeshQuery::SNavMeshQueryConfigInstant& queryConf, INavMeshQueryProcessing& queryProcessing, const size_t processingBatchSize = 1024) = 0;

#ifdef NAV_MESH_QUERY_DEBUG
		virtual void                             DebugDrawQueriesList() const = 0;
		virtual const INavMeshQueriesPtrSet&     GetQueriesDebug() const = 0;
#endif // NAV_MESH_QUERY_DEBUG

	protected:
		~INavMeshQueryManager() {}
	};
}