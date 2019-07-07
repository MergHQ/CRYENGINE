// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		struct IQueryBlueprint;

		//===================================================================================
		//
		// SQueryContext
		//
		// - bundles some stuff that is owned by a running query and is handed out to all query elements at execution time
		//
		//===================================================================================

		struct SQueryContext
		{
			explicit                            SQueryContext(const IQueryBlueprint& _queryBlueprint, const char* _szQuerierName, const CQueryID& _queryID, const Shared::IVariantDict& _globalParams, const IItemList* _pShuttledItems, const ITimeBudget& _timeBudget, IDebugRenderWorldPersistent* _pDebugRenderWorldPersistent, IDebugMessageCollection* _pDebugMessageCollection);

			const IQueryBlueprint&              queryBlueprint;              // the blueprint that was used to instantiate the live query; used for debugging purpose
			const char* const                   szQuerierName;               // name of the querier; used for debugging purpose; guaranteed to point to something non-NULL
			const CQueryID                      queryID;                     // unique ID identifying the live query
			const Shared::IVariantDict&         globalParams;                // global parameters that have been provided by the caller; Functions proving access to these global parameters use this structure to return the parameter values upon invocation
			const IItemList* const              pShuttledItems;              // resulting items of the previous query (in the chained query); might be a nullptr if there was no previous query
			const SItemIterationContext*        pItemIterationContext;       // this is only set during the evaluator phase when iterating through all generated items; it's a nullptr during the generation of items
			const ITimeBudget&                  timeBudget;                  // allows generators and deferred-evaluators to see if some of the granted time is still available (for when doing time-sliced work) or whether we've already run out of time
			IDebugRenderWorldPersistent* const  pDebugRenderWorldPersistent; // optional pointer to an IDebugRenderWorldPersisent for persisting debug-primitives; may be a nullptr if logging of the historic query is not desired
			const IDebugRenderWorldImmediate*   pDebugRenderWorldImmediate;  // optional pointer to an IDebugRenderWorldImmediate for drawing debug-primitives immediately (instead of persisting them in the historic query); may be a nullptr if debug-drawing is not desired
			IDebugMessageCollection*            pDebugMessageCollection;     // optional pointer to an IDebugMessageCollection for logging arbitrary messages and warnings; may be a nullptr if message-logging is not desired
		};

		inline SQueryContext::SQueryContext(const IQueryBlueprint& _queryBlueprint, const char* _szQuerierName, const CQueryID& _queryID, const Shared::IVariantDict& _globalParams, const IItemList* _pShuttledItems, const ITimeBudget& _timeBudget, IDebugRenderWorldPersistent* _pDebugRenderWorldPersistent, IDebugMessageCollection* _pDebugMessageCollection)
			: queryBlueprint(_queryBlueprint)
			, szQuerierName(_szQuerierName)
			, queryID(_queryID)
			, globalParams(_globalParams)
			, pShuttledItems(_pShuttledItems)
			, timeBudget(_timeBudget)
			, pItemIterationContext(nullptr)
			, pDebugRenderWorldPersistent(_pDebugRenderWorldPersistent)
			, pDebugRenderWorldImmediate(nullptr)
			, pDebugMessageCollection(_pDebugMessageCollection)
		{}

	}
}
