// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAISystem/NavigationSystem/INavMeshQuery.h>

namespace MNM
{
	struct INavMeshQueryManager
	{
	public:
		typedef std::unordered_set<const INavMeshQuery*> INavMeshQueriesPtrSet;

		virtual void                             Clear() = 0;
		virtual INavMeshQueryPtr                 CreateQuery(const INavMeshQuery::SNavMeshQueryConfig& queryConf) = 0;

#ifdef NAV_MESH_QUERY_DEBUG
		virtual void                             DebugDrawQueriesList() const = 0;
		virtual const INavMeshQueriesPtrSet&     GetQueriesDebug() const = 0;
#endif // NAV_MESH_QUERY_DEBUG

	protected:
		~INavMeshQueryManager() {}
	};
}