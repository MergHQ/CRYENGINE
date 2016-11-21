// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IItemList
		//
		// - an item list is owned by a "live" Query and handed out to the generator to populate it with items
		// - these items will then get evaluated and the best ones will get copied to the final result set
		//
		//===================================================================================

		struct IItemList
		{
			virtual                                ~IItemList() {}
			virtual void                           SetItemFactory(client::IItemFactory& itemFactory) = 0;
			virtual void                           CreateItemsByItemFactory(size_t numItemsToCreate) = 0;
			virtual size_t                         GetItemCount() const = 0;
			virtual client::IItemFactory&          GetItemFactory() const = 0;
			virtual void*                          GetItems() const = 0;
			virtual void                           CopyOtherToSelf(const IItemList& other) = 0;
		};

	}
}
