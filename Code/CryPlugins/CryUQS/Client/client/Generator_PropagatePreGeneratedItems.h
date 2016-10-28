// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
		// TODO: move/rename this file to CGenerator_PropagateShuttledItems.h (after the changes to this file have been submitted!)

		//===================================================================================
		//
		// CGenerator_PropagateShuttledItems<>
		//
		//===================================================================================

		template <class TItem>
		class CGenerator_PropagateShuttledItems : public CGeneratorBase<CGenerator_PropagateShuttledItems<TItem>, TItem>
		{
		private:
			typedef CGeneratorBase<CGenerator_PropagateShuttledItems<TItem>, TItem> BaseClass;

		public:

			struct SParams
			{
				// nothing

				UQS_EXPOSE_PARAMS_BEGIN
					// nothing
				UQS_EXPOSE_PARAMS_END
			};

			explicit CGenerator_PropagateShuttledItems(const SParams& params)
			{
				// nothing
			}

			typename BaseClass::EUpdateStatus DoUpdate(const typename BaseClass::SUpdateContext& updateContext, CItemListProxy_Writable<TItem>& itemListToPopulate)
			{
				if (!updateContext.blackboard.pShuttledItems)
				{
					updateContext.error.Format("CGenerator_PropagateShuttledItems<>::DoUpdate: updateContext.blackboard.pShuttledItems == NULL (this can happen if there was no previous query that forwarded its resulting items)");
					return BaseClass::EUpdateStatus::ExceptionOccurred;
				}

				core::IItemList& underlyingItemList = itemListToPopulate.GetUnderlyingItemList();

				// ensure both item lists have the same ItemFactory
				const IItemFactory& expectedItemFactory = underlyingItemList.GetItemFactory();
				const IItemFactory& receivedItemFactory = updateContext.blackboard.pShuttledItems->GetItemFactory();
				if (&expectedItemFactory != &receivedItemFactory)
				{
					updateContext.error.Format("CGenerator_PropagateShuttledItems<>::DoUpdate: incompatible item-factories (generator wants to propagate items of type '%s', but the blackboard contains items of type '%s')", expectedItemFactory.GetItemType().name(), receivedItemFactory.GetItemType().name());
					return BaseClass::EUpdateStatus::ExceptionOccurred;
				}

				// copy the items from the previous query to given output list
				underlyingItemList.CopyOtherToSelf(*updateContext.blackboard.pShuttledItems);

				return BaseClass::EUpdateStatus::FinishedGeneratingItems;
			}
		};

	}
}
