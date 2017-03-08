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

#if UQS_SCHEMATYC_SUPPORT
		client::IItemFactory* CUtils::FindItemFactoryBySchematycTypeName(const Schematyc::CTypeName& schematycTypeNameToSearchFor) const
		{
			auto it = m_schematycTypeName2itemFactory.find(schematycTypeNameToSearchFor);
			return (it == m_schematycTypeName2itemFactory.cend()) ? nullptr : it->second;
		}
#endif

		void CUtils::OnFactoryRegistered(client::IItemFactory* freshlyRegisteredFactory)
		{
			m_type2itemFactory[freshlyRegisteredFactory->GetItemType()] = freshlyRegisteredFactory;
#if UQS_SCHEMATYC_SUPPORT
			m_schematycTypeName2itemFactory[freshlyRegisteredFactory->GetItemType().GetSchematycTypeName()] = freshlyRegisteredFactory;
#endif
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
