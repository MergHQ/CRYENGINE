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

#if UQS_SCHEMATYC_SUPPORT
		void ConvertEntityIdToEntityIdWrapper(const EntityId& input, EntityIdWrapper& output)
		{
			output.value = input;
		}

		void ConvertEntityIdWrapperToEntityId(const EntityIdWrapper& input, EntityId& output)
		{
			output = input.value;
		}

		template <class TVec3BasedTypeWrapper>
		void ConvertVec3ToVec3BasedWrapper(const Vec3& input, TVec3BasedTypeWrapper& output)
		{
			output.value = input;
		}

		template <class TVec3BasedTypeWrapper>
		void ConvertVec3BasedWrapperToVec3(const TVec3BasedTypeWrapper& input, Vec3& output)
		{
			output = input.value;
		}
#endif

		void CStdLibRegistration::InstantiateItemFactoriesForRegistration()
		{
			// std::EntityId
			{
				client::SItemFactoryCallbacks<EntityIdWrapper> callbacks_EntityId;

				callbacks_EntityId.pAddItemToDebugRenderWorld = &EntityId_AddToDebugRenderWorld;
				callbacks_EntityId.pCreateItemDebugProxy = &EntityId_CreateItemDebugProxyForItem;

#if UQS_SCHEMATYC_SUPPORT
				callbacks_EntityId.itemConverters.AddFromForeignTypeConverter<EntityId, &ConvertEntityIdToEntityIdWrapper>("EntityId", "std::EntityIdWrapper", "345ae48a-ad37-4458-94b9-64806c2df087"_uqs_guid);
				callbacks_EntityId.itemConverters.AddToForeignTypeConverter<EntityId, &ConvertEntityIdWrapperToEntityId>("std::EntityIdWrapper", "EntityId", "82bd2238-dd8e-44ae-af30-a58e36f3b9a3"_uqs_guid);

				static const client::CItemFactory<EntityIdWrapper> itemFactory_EntityId("std::EntityId", "b53fd909-d454-4a5a-b125-3ca49cf7ffe5"_uqs_guid, "4b8dbf98-d5eb-42bf-a3f4-ec288d71422b"_uqs_guid, callbacks_EntityId);
#else
				static const client::CItemFactory<EntityIdWrapper> itemFactory_EntityId("std::EntityId", callbacks_EntityId);
#endif
			}

			// std::int
			{
				client::SItemFactoryCallbacks<int> callbacks_int;

				callbacks_int.pSerialize = &client::SerializeItem<int>;
				callbacks_int.pCreateDefaultObject = &GetValueInitializedItem<int>;

#if UQS_SCHEMATYC_SUPPORT
				static const client::CItemFactory<int> itemFactory_int("std::int", "7c3ddebe-a1f0-452b-b1ff-11191b03b3b4"_uqs_guid, "19a01e90-7cb8-4c1d-b8f7-208ebe3efb78"_uqs_guid, callbacks_int);
#else
				static const client::CItemFactory<int> itemFactory_int("std::int", callbacks_int);
#endif
			}

			// std::bool
			{
				client::SItemFactoryCallbacks<bool> callbacks_bool;

				callbacks_bool.pSerialize = &client::SerializeItem<bool>;
				callbacks_bool.pCreateDefaultObject = &GetValueInitializedItem<bool>;

#if UQS_SCHEMATYC_SUPPORT
				static const client::CItemFactory<bool> itemFactory_bool("std::bool", "ec49e7f2-57a1-4017-812c-d245ca289265"_uqs_guid, "488692a5-ac8e-409c-a707-3141fca4c458"_uqs_guid, callbacks_bool);
#else
				static const client::CItemFactory<bool> itemFactory_bool("std::bool", callbacks_bool);
#endif
			}

			// std::float
			{
				client::SItemFactoryCallbacks<float> callbacks_float;

				callbacks_float.pSerialize = &client::SerializeItem<float>;
				callbacks_float.pCreateDefaultObject = &GetValueInitializedItem<float>;

#if UQS_SCHEMATYC_SUPPORT
				static const client::CItemFactory<float> itemFactory_float("std::float", "9737adf8-408b-43ed-9263-c6f33a2bd78e"_uqs_guid, "3570e309-d6a9-4b9e-ae09-7616755eeca3"_uqs_guid, callbacks_float);
#else
				static const client::CItemFactory<float> itemFactory_float("std::float", callbacks_float);
#endif
			}

			// std::Pos3
			{
				client::SItemFactoryCallbacks<Pos3> callbacks_Pos3;

				callbacks_Pos3.pSerialize = &client::SerializeTypeWrappedItem<Pos3>;
				callbacks_Pos3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Pos3>;
				callbacks_Pos3.pAddItemToDebugRenderWorld = &Pos3_AddToDebugRenderWorld;
				callbacks_Pos3.pCreateItemDebugProxy = &Pos3_CreateItemDebugProxyForItem;

#if UQS_SCHEMATYC_SUPPORT
				callbacks_Pos3.itemConverters.AddFromForeignTypeConverter<Vec3, &ConvertVec3ToVec3BasedWrapper<Pos3>>("Vec3", "std::Pos3", "30aca024-1fff-4fb5-8956-cf54085c46dd"_uqs_guid);
				callbacks_Pos3.itemConverters.AddToForeignTypeConverter<Vec3, &ConvertVec3BasedWrapperToVec3<Pos3>>("std::Pos3", "Vec3", "9c90027d-0a4e-4420-9069-8797e69d37bc"_uqs_guid);

				static const client::CItemFactory<Pos3> itemFactory_Pos3("std::Pos3", "1e11a732-a95f-4399-9c83-0198e2ec3262"_uqs_guid, "3682383e-e146-4d0a-8917-2da8c0b34341"_uqs_guid, callbacks_Pos3);
#else
				static const client::CItemFactory<Pos3> itemFactory_Pos3("std::Pos3", callbacks_Pos3);
#endif
			}

			// std::Ofs3
			{
				client::SItemFactoryCallbacks<Ofs3> callbacks_Ofs3;

				callbacks_Ofs3.pSerialize = &client::SerializeTypeWrappedItem<Ofs3>;
				callbacks_Ofs3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Ofs3>;

#if UQS_SCHEMATYC_SUPPORT
				callbacks_Ofs3.itemConverters.AddFromForeignTypeConverter<Vec3, &ConvertVec3ToVec3BasedWrapper<Ofs3>>("Vec3", "std::Ofs3", "f36a4dde-2995-4f96-bda4-f8546b0074a9"_uqs_guid);
				callbacks_Ofs3.itemConverters.AddToForeignTypeConverter<Vec3, &ConvertVec3BasedWrapperToVec3<Ofs3>>("std::Ofs3", "Vec3", "5ff51b20-affa-4bf1-9bed-ec478a3c4f8c"_uqs_guid);

				static const client::CItemFactory<Ofs3> itemFactory_Ofs3("std::Ofs3", "a20409f2-93bd-4a89-bc20-b99d87dc4a1d"_uqs_guid, "9e01e609-3676-442e-a526-c3cd3f65a396"_uqs_guid, callbacks_Ofs3);
#else
				static const client::CItemFactory<Ofs3> itemFactory_Ofs3("std::Ofs3", callbacks_Ofs3);
#endif
			}

			// std::Dir3
			{
				client::SItemFactoryCallbacks<Dir3> callbacks_Dir3;

				callbacks_Dir3.pSerialize = &client::SerializeTypeWrappedItem<Dir3>;
				callbacks_Dir3.pCreateDefaultObject = &GetVec3BasedTypeWrapperZero<Dir3>;

#if UQS_SCHEMATYC_SUPPORT
				callbacks_Dir3.itemConverters.AddFromForeignTypeConverter<Vec3, &ConvertVec3ToVec3BasedWrapper<Dir3>>("Vec3", "std::Dir3", "95e16187-ac55-4dd7-80dd-0ffa42edbcdc"_uqs_guid);
				callbacks_Dir3.itemConverters.AddToForeignTypeConverter<Vec3, &ConvertVec3BasedWrapperToVec3<Dir3>>("std::Dir3", "Vec3", "f525d01e-272e-4f38-a480-ab119d12b766"_uqs_guid);

				static const client::CItemFactory<Dir3> itemFactory_Dir3("std::Dir3", "1f78ca86-1627-41ad-990b-ca0f9dfc0879"_uqs_guid, "e369872f-5c0f-4e38-9e99-106a25fe1f3e"_uqs_guid, callbacks_Dir3);
#else
				static const client::CItemFactory<Dir3> itemFactory_Dir3("std::Dir3", callbacks_Dir3);
#endif
			}

			// std::NavigationAgentTypeID
			{
				client::SItemFactoryCallbacks<NavigationAgentTypeID> callbacks_NavigationAgentTypeID;

				callbacks_NavigationAgentTypeID.pSerialize = &NavigationAgentTypeID_Serialize;

#if UQS_SCHEMATYC_SUPPORT
				static const client::CItemFactory<NavigationAgentTypeID> itemFactory_NavigationAgentTypeID("std::NavigationAgentTypeID", "cdb0111a-a042-41ca-9d4b-b52816a2ffa0"_uqs_guid, "3d700e90-b5b6-47d2-bfb8-654458d462dc"_uqs_guid, callbacks_NavigationAgentTypeID);
#else
				static const client::CItemFactory<NavigationAgentTypeID> itemFactory_NavigationAgentTypeID("std::NavigationAgentTypeID", callbacks_NavigationAgentTypeID);
#endif
			}
		}

	}
}
