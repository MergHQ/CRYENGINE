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
			// std::EntityId
			{
				client::SItemFactoryCallbacks<EntityIdWrapper> callbacks_EntityId;

				callbacks_EntityId.pAddItemToDebugRenderWorld = &EntityId_AddToDebugRenderWorld;
				callbacks_EntityId.pCreateItemDebugProxy = &EntityId_CreateItemDebugProxyForItem;

				static const client::CItemFactory<EntityIdWrapper> itemFactory_EntityId("std::EntityId", callbacks_EntityId);
			}

			// std::int
			{
				client::SItemFactoryCallbacks<int> callbacks_int;

				callbacks_int.pSerialize = &Int_Serialize;
				callbacks_int.pCreateDefaultObject = &GetValueInitializedItem<int>;

				static const client::CItemFactory<int> itemFactory_int("std::int", callbacks_int);
			}

			// std::bool
			{
				client::SItemFactoryCallbacks<bool> callbacks_bool;

				callbacks_bool.pSerialize = &Bool_Serialize;
				callbacks_bool.pCreateDefaultObject = &GetValueInitializedItem<bool>;

				static const client::CItemFactory<bool> itemFactory_bool("std::bool", callbacks_bool);
			}

			// std::float
			{
				client::SItemFactoryCallbacks<float> callbacks_float;

				callbacks_float.pSerialize = &Float_Serialize;
				callbacks_float.pCreateDefaultObject = &GetValueInitializedItem<float>;

				static const client::CItemFactory<float> itemFactory_float("std::float", callbacks_float);
			}

			// std::Vec3
			{
				client::SItemFactoryCallbacks<Vec3> callbacks_Vec3;

				callbacks_Vec3.pSerialize = &Vec3_Serialize;
				callbacks_Vec3.pCreateDefaultObject = &GetVec3Zero;
				callbacks_Vec3.pCreateItemDebugProxy = &Vec3_CreateItemDebugProxyForItem;
				//#error maybe do NOT force our Vec3 debug-proxy function on the client?
				//#error alternative idea: provide a way such that the client can override already provided callbacks? (might require some type casts geared around SItemFactoryCallbacks<T>)
				// FIXME: no debug-rendering? on the other hand: is it good to have a debug-proxy (Vec3_CreateItemDebugProxyForItem) for it? after all, a Vec3 item could be anything: position, direction, force, etc.

				static const client::CItemFactory<Vec3> itemFactory_Vec3("std::Vec3", callbacks_Vec3);
			}

			// std::NavigationAgentTypeID
			{
				client::SItemFactoryCallbacks<NavigationAgentTypeID> callbacks_NavigationAgentTypeID;

				callbacks_NavigationAgentTypeID.pSerialize = &NavigationAgentTypeID_Serialize;

				static const client::CItemFactory<NavigationAgentTypeID> itemFactory_NavigationAgentTypeID("std::NavigationAgentTypeID", callbacks_NavigationAgentTypeID);
			}
		}

	}
}
