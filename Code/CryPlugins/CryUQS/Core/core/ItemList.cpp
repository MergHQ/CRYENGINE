// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ItemList.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		CItemList::CItemList()
			: m_pItemFactory(nullptr)
			, m_pItems(nullptr)
			, m_numItems(0)
		{
			// nothing
		}

		CItemList::~CItemList()
		{
			if (m_pItemFactory)
			{
				m_pItemFactory->DestroyItems(m_pItems);
			}
		}

		void CItemList::CreateItemsByItemFactory(size_t numItemsToCreate)
		{
			// ensure SetItemFactory() has been called prior
			CRY_ASSERT(m_pItemFactory);

			// ensure that no items have been created yet (we don't support recycling the item list)
			CRY_ASSERT(!m_pItems);

			m_pItems = m_pItemFactory->CreateItems(numItemsToCreate, Client::IItemFactory::EItemInitMode::UseDefaultConstructor);
			m_numItems = numItemsToCreate;
		}

		void CItemList::CloneItems(const void* pOriginalItems, size_t numItemsToClone)
		{
			CRY_ASSERT(pOriginalItems);

			// ensure SetItemFactory() has been called prior
			CRY_ASSERT(m_pItemFactory);

			// ensure that no items have been created yet (we don't support recycling the item list)
			CRY_ASSERT(!m_pItems);

			m_pItems = m_pItemFactory->CloneItems(pOriginalItems, numItemsToClone);
			m_numItems = numItemsToClone;
		}

		size_t CItemList::GetItemCount() const
		{
			return m_numItems;
		}

		Client::IItemFactory& CItemList::GetItemFactory() const
		{
			CRY_ASSERT(m_pItemFactory);    // CreateItemsByItemFactory() should have been called before; I cannot see a use-case where it would make sense the other way round
			return *m_pItemFactory;
		}

		void* CItemList::GetItems() const
		{
			return m_pItems;
		}

		void CItemList::CopyOtherToSelf(const IItemList& other)
		{
			if (m_pItemFactory)
			{
				m_pItemFactory->DestroyItems(m_pItems);
			}

			m_pItemFactory = &other.GetItemFactory();
			m_pItems = m_pItemFactory->CloneItems(other.GetItems(), other.GetItemCount());
			m_numItems = other.GetItemCount();
		}

		void CItemList::SetItemFactory(Client::IItemFactory& itemFactory)
		{
			CRY_ASSERT(!m_pItemFactory);  // changing the item-factory is not supported (which use-case would it tackle?)
			m_pItemFactory = &itemFactory;
		}

		void CItemList::CopyOtherToSelfViaIndexList(const IItemList& other, const size_t* pIndexes, size_t numIndexes)
		{
			if (m_pItemFactory)
			{
				m_pItemFactory->DestroyItems(m_pItems);
			}

			m_pItemFactory = &other.GetItemFactory();
			m_pItems = m_pItemFactory->CloneItemsViaIndexList(other.GetItems(), pIndexes, numIndexes);
			m_numItems = numIndexes;
		}

		void* CItemList::GetItemAtIndex(size_t index) const
		{
			CRY_ASSERT(m_pItemFactory);
			CRY_ASSERT(m_pItems);
			CRY_ASSERT(index < m_numItems);
			return m_pItemFactory->GetItemAtIndex(m_pItems, index);
		}

		size_t CItemList::GetMemoryUsedSafe() const
		{
			if (m_pItemFactory)
			{
				return m_numItems * m_pItemFactory->GetItemSize();
			}
			else
			{
				return 0;
			}
		}

	}
}
