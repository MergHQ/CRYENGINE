// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CQueryResultSet
		//
		//===================================================================================

		CQueryResultSet::CQueryResultSet(const IItemList& generatedItems, std::vector<size_t>&& itemIndexes, std::vector<float>&& itemScores)
			: m_scores(std::move(itemScores))
		{
			CRY_ASSERT(itemIndexes.size() == m_scores.size());

			const size_t numIndexes = itemIndexes.size();
			const size_t* pIndexes = (numIndexes > 0) ? &itemIndexes.front() : nullptr;

			m_items.CopyOtherToSelfViaIndexList(generatedItems, pIndexes, numIndexes);
		}

		Client::IItemFactory& CQueryResultSet::GetItemFactory() const
		{
			return m_items.GetItemFactory();
		}

		size_t CQueryResultSet::GetResultCount() const
		{
			CRY_ASSERT(m_items.GetItemCount() == m_scores.size());

			return m_items.GetItemCount();
		}

		IQueryResultSet::SResultSetEntry CQueryResultSet::GetResult(size_t index) const
		{
			CRY_ASSERT(m_items.GetItemCount() == m_scores.size());
			CRY_ASSERT(index < m_items.GetItemCount());

			void* pItem = m_items.GetItemAtIndex(index);
			float score = m_scores[index];
			return SResultSetEntry(pItem, score);
		}

		const CQueryResultSet& CQueryResultSet::GetImplementation() const
		{
			return *this;
		}

		const CItemList& CQueryResultSet::GetItemList() const
		{
			return m_items;
		}

		void CQueryResultSet::DeleteSelf()
		{
			delete this;
		}

	}
}
