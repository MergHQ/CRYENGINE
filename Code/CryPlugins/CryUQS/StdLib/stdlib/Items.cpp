// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CrySerialization/Math.h>

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

		// for client::STypeWrapper<> types whose underlying type is Vec3
		template <class TVec3BasedTypeWrapper>
		TVec3BasedTypeWrapper GetVec3BasedTypeWrapperZero()
		{
			return TVec3BasedTypeWrapper(Vec3(ZERO));
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

				callbacks_int.pSerialize = &client::SerializeItem<int>;
				callbacks_int.pCreateDefaultObject = &GetValueInitializedItem<int>;

				static const client::CItemFactory<int> itemFactory_int("std::int", callbacks_int);
			}

			// std::bool
			{
				client::SItemFactoryCallbacks<bool> callbacks_bool;

				callbacks_bool.pSerialize = &client::SerializeItem<bool>;
				callbacks_bool.pCreateDefaultObject = &GetValueInitializedItem<bool>;

				static const client::CItemFactory<bool> itemFactory_bool("std::bool", callbacks_bool);
			}

			// std::float
			{
				client::SItemFactoryCallbacks<float> callbacks_float;

				callbacks_float.pSerialize = &client::SerializeItem<float>;
				callbacks_float.pCreateDefaultObject = &GetValueInitializedItem<float>;

				static const client::CItemFactory<float> itemFactory_float("std::float", callbacks_float);
			}

			// std::Pos3
			{
				client::SItemFactoryCallbacks<Pos3> callbacks_Pos3;

				callbacks_Pos3.pSerialize = &client::SerializeTypeWrappedItem<Pos3>;
				callbacks_Pos3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Pos3>;
				callbacks_Pos3.pAddItemToDebugRenderWorld = &Pos3_AddToDebugRenderWorld;
				callbacks_Pos3.pCreateItemDebugProxy = &Pos3_CreateItemDebugProxyForItem;

				static const client::CItemFactory<Pos3> itemFactory_Pos3("std::Pos3", callbacks_Pos3);
			}

			// std::Ofs3
			{
				client::SItemFactoryCallbacks<Ofs3> callbacks_Ofs3;

				callbacks_Ofs3.pSerialize = &client::SerializeTypeWrappedItem<Ofs3>;
				callbacks_Ofs3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Ofs3>;

				static const client::CItemFactory<Ofs3> itemFactory_Ofs3("std::Ofs3", callbacks_Ofs3);
			}

			// std::Dir3
			{
				client::SItemFactoryCallbacks<Dir3> callbacks_Dir3;

				callbacks_Dir3.pSerialize = &client::SerializeTypeWrappedItem<Dir3>;
				callbacks_Dir3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Dir3>;

				static const client::CItemFactory<Dir3> itemFactory_Dir3("std::Dir3", callbacks_Dir3);
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
