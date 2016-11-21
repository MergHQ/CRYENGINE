// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// SQueryBlackboard
		//
		// - bundles some stuff that is owned by a running query and is handed out to all sub components at execution time
		// - the running query writes to the blackboard whereas functions, generators and evaluators read from it
		//
		//===================================================================================

		struct SQueryBlackboard
		{
			explicit                       SQueryBlackboard(const shared::IVariantDict& _globalParams, const IItemList* _pShuttledItems, IDebugRenderWorld* _pDebugRenderWorld);

			const shared::IVariantDict&    globalParams;
			const IItemList* const         pShuttledItems;           // resulting items of the previous query (in the chained query); might be a nullptr if there was no previous query
			const SItemIterationContext*   pItemIterationContext;    // this is only set during the evaluator phase when iterating through all generated items; it's a nullptr during the generation of items
			IDebugRenderWorld* const       pDebugRenderWorld;        // optional pointer to an IDebugRenderWorld for persisting debug-primitives; may be a nullptr if no debug-rendering is desired
		};

		inline SQueryBlackboard::SQueryBlackboard(const shared::IVariantDict& _globalParams, const IItemList* _pShuttledItems, IDebugRenderWorld* _pDebugRenderWorld)
			: globalParams(_globalParams)
			, pShuttledItems(_pShuttledItems)
			, pItemIterationContext(nullptr)
			, pDebugRenderWorld(_pDebugRenderWorld)
		{}

	}
}
