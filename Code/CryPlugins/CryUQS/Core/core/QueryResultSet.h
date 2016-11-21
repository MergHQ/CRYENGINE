// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CQueryResultSet
		//
		//===================================================================================

		class CQueryResultSet : public IQueryResultSet
		{
		public:
			explicit                          CQueryResultSet();

			// IQueryResultSet
			virtual client::IItemFactory&     GetItemFactory() const override;
			virtual size_t                    GetResultCount() const override;
			virtual SResultSetEntry           GetResult(size_t index) const override;
			// ~IQueryResultSet

			void                              SetItemFactoryAndCreateItems(client::IItemFactory& itemFactory, size_t numItemsToCreate);
			void                              SetItemAndScore(size_t index, const void* pSourceItem, float score);

		private:
			                                  UQS_NON_COPYABLE(CQueryResultSet);

			// IQueryResultSet
			virtual void                      DeleteSelf() override;
			// ~IQueryResultSet

		private:
			client::IItemFactory*             m_pItemFactory;
			CItemList                         m_items;
			std::vector<float>                m_scores;
		};

	}
}
