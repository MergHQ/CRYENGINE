// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// SItemIterationContext
		//
		// - passed to each function during the evaluation phase
		// - during the generator phase, a nullptr will be passed instead
		//
		//===================================================================================

		struct SItemIterationContext
		{
			explicit                 SItemIterationContext(const IItemList& _items);
			const IItemList&         generatedItems;        // the list of items a generator has produced and that we will iterate on in the evaluation phase
		};

		inline SItemIterationContext::SItemIterationContext(const IItemList& _generatedItems)
			: generatedItems(_generatedItems)
		{}

	}
}
