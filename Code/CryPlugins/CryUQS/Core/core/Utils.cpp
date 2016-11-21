// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		client::IItemFactory* CUtils::FindItemFactoryByType(const shared::CTypeInfo& type) const
		{
			auto it = m_type2itemFactory.find(type);
			return (it == m_type2itemFactory.cend()) ? nullptr : it->second;
		}

		void CUtils::OnFactoryRegistered(client::IItemFactory* freshlyRegisteredFactory)
		{
			m_type2itemFactory[freshlyRegisteredFactory->GetItemType()] = freshlyRegisteredFactory;
		}

		void CUtils::SubscribeToStuffInHub(CHub& hub)
		{
			hub.GetItemFactoryDatabase().RegisterListener(this);
		}

		void CUtils::UnsubscribeFromStuffInHub(CHub& hub)
		{
			hub.GetItemFactoryDatabase().UnregisterListener(this);
		}

	}
}
