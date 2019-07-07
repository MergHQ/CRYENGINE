// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryCore/functor.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Client
	{

		//===================================================================================
		//
		// SQueryRequest
		//
		//===================================================================================

		struct SQueryRequest
		{
			explicit                              SQueryRequest(const Core::CQueryBlueprintID& _queryBlueprintID, const Shared::IVariantDict& _runtimeParams, const char* _querierName, const Functor1<const Core::SQueryResult&>& _callback, int _priority);

			static const int                      kDefaultPriority = 1;

			Core::CQueryBlueprintID               queryBlueprintID;         // the query-blueprint which is stored in the library under this ID will be instantiated by the IQueryManager
			const Shared::IVariantDict&           runtimeParams;            // key/value pairs set by the caller at runtime
			const char*                           szQuerierName;            // name of whoever requested to start this query; used  for debugging purposes only
			Functor1<const Core::SQueryResult&>   callback;                 // optional callback for when the query finishes; does *not* get called when the query couldn't even be started due to some error
			int                                   priority;                 // priority (1-based) to specify how "important" a query is; the higher the priority the more time budget a query will be granted to run; e. g. a value of 6 means: 6 times more of the time budget than a query with priority = 1 (and 3 times more time budget than with a priority of 2, etc.)
		};

		inline SQueryRequest::SQueryRequest(const Core::CQueryBlueprintID& _queryBlueprintID, const Shared::IVariantDict& _runtimeParams, const char* _szQuerierName, const Functor1<const Core::SQueryResult&>& _callback, int _priority)
			: queryBlueprintID(_queryBlueprintID)
			, runtimeParams(_runtimeParams)
			, szQuerierName(_szQuerierName)
			, callback(_callback)
			, priority(_priority)
		{}

	}
}
