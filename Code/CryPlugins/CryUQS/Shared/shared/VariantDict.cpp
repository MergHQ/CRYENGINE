// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "VariantDict.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace shared
	{

		CVariantDict::SDataEntry::SDataEntry()
			: pItemFactory(nullptr)
			, pObject(nullptr)
		{
			// nothing
		}

		CVariantDict::CVariantDict()
		{
			// nothing
		}

		CVariantDict::~CVariantDict()
		{
			Clear();
		}

		void CVariantDict::__AddOrReplace(const char* key, client::IItemFactory& itemFactory, void* pObject)
		{
			SDataEntry& entry = m_dataItems[key];

			assert((entry.pItemFactory && entry.pObject) || (!entry.pItemFactory && !entry.pObject));

			if(entry.pItemFactory)
			{
				entry.pItemFactory->DestroyItems(entry.pObject);
			}

			entry.pItemFactory = &itemFactory;
			entry.pObject = pObject;
		}

		void CVariantDict::AddSelfToOtherAndReplace(IVariantDict& out) const
		{
			for (const auto& pair : m_dataItems)
			{
				const char* key = pair.first.c_str();
				const SDataEntry& entry = pair.second;

				assert(entry.pItemFactory && entry.pObject);

				void* clonedSingleObject = entry.pItemFactory->CloneItem(entry.pObject);
				out.__AddOrReplace(key, *entry.pItemFactory, clonedSingleObject);
			}
		}

		bool CVariantDict::Exists(const char* key) const
		{
			return (m_dataItems.find(key) != m_dataItems.cend());
		}

		client::IItemFactory* CVariantDict::FindItemFactory(const char* key) const
		{
			auto it = m_dataItems.find(key);
			return (it == m_dataItems.cend()) ? nullptr : it->second.pItemFactory;
		}

		bool CVariantDict::FindItemFactoryAndObject(const char* key, client::IItemFactory* &outItemItemFactory, void* &outObject) const
		{
			auto it = m_dataItems.find(key);
			if (it != m_dataItems.cend())
			{
				outItemItemFactory = it->second.pItemFactory;
				outObject = it->second.pObject;
				return true;
			}
			else
			{
				return false;
			}
		}

		const std::map<string, CVariantDict::SDataEntry>& CVariantDict::GetEntries() const
		{
			return m_dataItems;
		}

		void CVariantDict::Clear()
		{
			for (auto pair : m_dataItems)
			{
				SDataEntry& entry = pair.second;
				assert(entry.pItemFactory && entry.pObject);
				entry.pItemFactory->DestroyItems(entry.pObject);
			}
			m_dataItems.clear();
		}

	}
}
