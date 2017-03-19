// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		class CHub;

		//===================================================================================
		//
		// CUtils
		//
		//===================================================================================

		class CUtils : public IUtils, public IFactoryDatabaseListener<client::IItemFactory>
		{
		public:
			// IUtils
			virtual client::IItemFactory*                           FindItemFactoryByType(const shared::CTypeInfo& type) const override;
			// ~IUtils

			// IFactoryDatabaseListener<client::IItemFactory>
			virtual void                                            OnFactoryRegistered(client::IItemFactory* freshlyRegisteredFactory) override;
			// ~IFactoryDatabaseListener<client::IItemFactory>

			// - called by CHub's ctor + dtor
			// - this is to ensure that all member variables in CHub are definitely constructed before giving CUtils access to them
			void                                                    SubscribeToStuffInHub(CHub& hub);
			void                                                    UnsubscribeFromStuffInHub(CHub& hub);

		private:
			std::map<shared::CTypeInfo, client::IItemFactory*>      m_type2itemFactory;
		};

	}
}
