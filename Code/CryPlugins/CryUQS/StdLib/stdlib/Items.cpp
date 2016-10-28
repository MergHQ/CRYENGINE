// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace stdlib
	{

		template <typename T>
		T GetValueInitializedItem()
		{
			return T();
		}

		Vec3 GetVec3Zero()
		{
			return Vec3(ZERO);
		}

		void CStdLibRegistration::InstantiateItemFactoriesForRegistration()
		{
			static const client::CItemFactory<EntityIdWrapper, nullptr, &EntityId_AddToDebugRenderWorld, &EntityId_CreateItemDebugProxyForItem> itemFactory_EntityId("std::EntityId", nullptr);
			static const client::CItemFactory<int, &Int_Serialize, nullptr, nullptr> itemFactory_int("std::int", &GetValueInitializedItem<int>);
			static const client::CItemFactory<bool, &Bool_Serialize, nullptr, nullptr> itemFactory_bool("std::bool", &GetValueInitializedItem<bool>);
			static const client::CItemFactory<float, &Float_Serialize, nullptr, nullptr> itemFactory_float("std::float", &GetValueInitializedItem<float>);
			static const client::CItemFactory<Vec3, &Vec3_Serialize, nullptr, &Vec3_CreateItemDebugProxyForItem> itemFactory_Vec3("std::Vec3", &GetVec3Zero);
			static const client::CItemFactory<NavigationAgentTypeID, &NavigationAgentTypeID_Serialize, nullptr, nullptr> itemFactory_NavigationAgentTypeID("std::NavigationAgentTypeID", nullptr);
		}

	}
}
