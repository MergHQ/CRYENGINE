// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// SQueryRequest
		//
		//===================================================================================

		struct SQueryRequest
		{
			explicit                              SQueryRequest(const char* _queryBlueprintName, const shared::IVariantDict& _runtimeParams, const char* _querierName, const Functor1<const core::SQueryResult&>& _callback);

			const char*                           queryBlueprintName;       // the query-blueprint with this name will be instantiated by the IQueryManager
			const shared::IVariantDict&           runtimeParams;            // key/value pairs set by the caller at runtime
			const char*                           querierName;              // name of whoever requested to start this query; used  for debugging purposes only
			Functor1<const core::SQueryResult&>   callback;                 // optional callback for when the query finishes; does *not* get called when the query couldn't even be started due to some error
		};

		inline SQueryRequest::SQueryRequest(const char* _queryBlueprintName, const shared::IVariantDict& _runtimeParams, const char* _querierName, const Functor1<const core::SQueryResult&>& _callback)
			: queryBlueprintName(_queryBlueprintName)
			, runtimeParams(_runtimeParams)
			, querierName(_querierName)
			, callback(_callback)
		{}

	}
}
