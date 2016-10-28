// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// IItemFactory
		//
		//===================================================================================

		struct IItemFactory
		{
			enum class EItemInitMode
			{
				UseDefaultConstructor,
				UseUserProvidedFunction
			};

			virtual                             ~IItemFactory() {}
			virtual void*                       CreateItems(size_t numItems, EItemInitMode itemInitMode) = 0;
			virtual void*                       CloneItem(const void* pOriginalItem) = 0;
			virtual void                        DestroyItems(void* pItems) = 0;
			virtual const shared::CTypeInfo&    GetItemType() const = 0;
			virtual size_t                      GetItemSize() const = 0;
			virtual void                        CopyItem(void* pTargetItem, const void* pSourceItem) const = 0;
			virtual void*                       GetItemAtIndex(void* pItems, size_t index) const = 0;
			virtual const void*                 GetItemAtIndex(const void* pItems, size_t index) const = 0;
			virtual const char*                 GetName() const = 0;
			virtual void                        AddItemToDebugRenderWorld(const void* pItem, core::IDebugRenderWorld& debugRW) const = 0;
			virtual void                        CreateItemDebugProxyForItem(const void* pItem, core::IItemDebugProxyFactory& itemDebugProxyFactory) const = 0;

			virtual bool                        CanBePersistantlySerialized() const = 0;
			virtual bool                        TrySerializeItem(const void* pItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const = 0;
			virtual bool                        TryDeserializeItem(void* pOutItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const = 0;
			virtual bool                        TryDeserializeItemIntoDict(shared::IVariantDict& out, const char* szKey, Serialization::IArchive& archive, const char* szName, const char* szLabel) = 0;
		};

	}
}
