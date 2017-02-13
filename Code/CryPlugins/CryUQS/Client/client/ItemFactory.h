// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// SItemFactoryCallbacks<>
		//
		//===================================================================================

		template <class TItem>
		struct SItemFactoryCallbacks
		{
			typedef bool                      (*serializeFunc_t)(Serialization::IArchive& archive, TItem& item, const char* szName, const char* szLabel);
			typedef TItem                     (*createDefaultObjectFunc_t)();
			typedef void                      (*addItemToDebugRenderWorldFunc_t)(const TItem& item, core::IDebugRenderWorldPersistent& debugRW);
			typedef void                      (*createItemDebugProxyFunc_t)(const TItem& item, core::IItemDebugProxyFactory& itemDebugProxyFactory);

			// - a serialization function is used to convert the item to and from its string representation
			// - it's used by the query editor to save literals to a query blueprint
			// - typically, there should be such a function for int, bool, float, Vec3, etc., but usually not for pointers and the like
			serializeFunc_t                   pSerialize;

			// - user-defined function that creates an item with meaningful "default" values
			// - this function usually gets provided for items whose constructor is bypassing initialization (e. g. Vec3's values are undefined by default)
			createDefaultObjectFunc_t         pCreateDefaultObject;

			// - an optional function that will add some debug-primitives to a given IDebugRenderWorldPersistent to represent the item in its visual form
			// - this is typically *not* used for built-in types, such as int, bool, float, etc. because they can have different meanings in different contexts
			// - but if, for example, the item is an area, then this function would add debug-primitives to draw such an area
			addItemToDebugRenderWorldFunc_t   pAddItemToDebugRenderWorld;

			// - optional function to create a debug-proxy of the item
			// - it's used by the query history to figure out what the closest item to the camera is, in order to show details about that particular item
			createItemDebugProxyFunc_t        pCreateItemDebugProxy;

			explicit                          SItemFactoryCallbacks();
		};

		template <class TItem>
		SItemFactoryCallbacks<TItem>::SItemFactoryCallbacks()
			: pSerialize(nullptr)
			, pCreateDefaultObject(nullptr)
			, pAddItemToDebugRenderWorld(nullptr)
			, pCreateItemDebugProxy(nullptr)
		{}


		//-----------------------------------------------------------------------------------
		// SerializeItem<>() - standard serialize function for items whose data types are already supported by yasli out-of-the-box
		//-----------------------------------------------------------------------------------

		template <class TItem>
		bool SerializeItem(Serialization::IArchive& archive, TItem& item, const char* szName, const char* szLabel)
		{
			return archive(item, szName, szLabel);
		}

		//-----------------------------------------------------------------------------------
		// SerializeTypeWrappedItem<>() - serialize function for items of type uqs::client::STypeWrapper<> whose underlying type is already supported by yasli out-of-the-box 
		//-----------------------------------------------------------------------------------

		template <class TTypeWrapper>
		bool SerializeTypeWrappedItem(Serialization::IArchive& archive, TTypeWrapper& item, const char* szName, const char* szLabel)
		{
			return archive(item.value, szName, szLabel);
		}

		namespace internal
		{

			//===================================================================================
			//
			// CItemFactoryBase
			//
			//===================================================================================

			class CItemFactoryBase : public IItemFactory, public CFactoryBase<CItemFactoryBase>
			{
			public:
				// IItemFactory
				virtual const char*                  GetName() const override;
				// ~IItemFactory

				// IItemFactory: forward these pure virtual methods to the derived class
				virtual void*                        CreateItems(size_t numItems, EItemInitMode itemInitMode) override = 0;
				virtual void*                        CloneItem(const void* pOriginalItem) override = 0;
				virtual void                         DestroyItems(void* pItems) override = 0;
				virtual const shared::CTypeInfo&     GetItemType() const override = 0;
				virtual size_t                       GetItemSize() const override = 0;
				virtual void                         CopyItem(void* pTargetItem, const void* pSourceItem) const override = 0;
				virtual void*                        GetItemAtIndex(void* pItems, size_t index) const override = 0;
				virtual const void*                  GetItemAtIndex(const void* pItems, size_t index) const override = 0;
				virtual void                         AddItemToDebugRenderWorld(const void* pItem, core::IDebugRenderWorldPersistent& debugRW) const override = 0;
				virtual bool                         CanBeRepresentedInDebugRenderWorld() const override = 0;
				virtual void                         CreateItemDebugProxyForItem(const void* pItem, core::IItemDebugProxyFactory& itemDebugProxyFactory) const override = 0;
				virtual bool                         CanBePersistantlySerialized() const override = 0;
				virtual bool                         TrySerializeItem(const void* pItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const override = 0;
				virtual bool                         TryDeserializeItem(void* pOutItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const override = 0;
				virtual bool                         TryDeserializeItemIntoDict(shared::IVariantDict& out, const char* szKey, Serialization::IArchive& archive, const char* szName, const char* szLabel) override = 0;
				// ~IItemFactory

			protected:
				explicit                             CItemFactoryBase(const char* name);
			};

			inline CItemFactoryBase::CItemFactoryBase(const char* name)
				: CFactoryBase(name)
			{}

			inline const char* CItemFactoryBase::GetName() const
			{
				return CFactoryBase::GetName();
			}

			//===================================================================================
			//
			// CItemFactoryInternal<>
			//
			//===================================================================================

			template <class TItem>
			class CItemFactoryInternal final : public CItemFactoryBase
			{
			private:

				//===================================================================================
				//
				// SHeader
				//
				// - header that resides before the item-array in memory
				// - it's used to track how many items have been created in one large memory block via CreateItems()
				// - there's a special case for when creating (and destroying) exactly 1 item: this header will then be used to manage a free list of single items
				//
				//===================================================================================

				struct SHeader
				{
					size_t                          itemCount;
					SHeader*                        pNextInFreeListOfSingleItems;    // item-arrays with exactly 1 element are managed as a free list
					TItem                           items[1];

				private:
					// instances of this struct are not supported since it would mean that the one item in .items[1] gets created, which we don't want here
					                                SHeader();
													SHeader(const SHeader&);
													SHeader(SHeader&&);
													~SHeader();
					SHeader&                        operator=(const SHeader&);
					SHeader&                        operator=(SHeader&&);
				};

			public:

				explicit                            CItemFactoryInternal(const char* name, const SItemFactoryCallbacks<TItem>& callbacks, bool bAutoRegisterBuiltinFunctions);
													~CItemFactoryInternal();

				// IItemFactory
				virtual void*                       CreateItems(size_t numItems, EItemInitMode itemInitMode) override;
				virtual void*                       CloneItem(const void* p) override;
				virtual void                        DestroyItems(void* p) override;
				virtual const shared::CTypeInfo&    GetItemType() const override;
				virtual size_t                      GetItemSize() const override;
				virtual void                        CopyItem(void* pTargetItem, const void* pSourceItem) const override;
				virtual void*                       GetItemAtIndex(void* pItems, size_t index) const override;
				virtual const void*                 GetItemAtIndex(const void* pItems, size_t index) const override;
				virtual void                        AddItemToDebugRenderWorld(const void* item, core::IDebugRenderWorldPersistent& debugRW) const override;
				virtual bool                        CanBeRepresentedInDebugRenderWorld() const override;
				virtual void                        CreateItemDebugProxyForItem(const void* item, core::IItemDebugProxyFactory& itemDebugProxyFactory) const override;
				virtual bool                        CanBePersistantlySerialized() const override;
				virtual bool                        TrySerializeItem(const void* pItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const override;
				virtual bool                        TryDeserializeItem(void* pOutItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const override;
				virtual bool                        TryDeserializeItemIntoDict(shared::IVariantDict& out, const char* szKey, Serialization::IArchive& archive, const char* szName, const char* szLabel) override;
				// ~IItemFactory

			private:

				static size_t                       ComputeMemoryRequiredForSingleItem();
				static size_t                       ComputeMemoryRequired(size_t itemCount);
				static TItem*                       AllocateUninitializedMemoryForItems(size_t numItems);

			private:

				const SItemFactoryCallbacks<TItem>  m_callbacks;

				static const ptrdiff_t              s_itemsOffsetInHeader = offsetof(SHeader, items);
				static SHeader*                     s_pFreeListHoldingSingleItems;
			};

			template <class TItem>
			typename CItemFactoryInternal<TItem>::SHeader* CItemFactoryInternal<TItem>::s_pFreeListHoldingSingleItems;

			template <class TItem>
			CItemFactoryInternal<TItem>::CItemFactoryInternal(const char* name, const SItemFactoryCallbacks<TItem>& callbacks, bool bAutoRegisterBuiltinFunctions)
				: CItemFactoryBase(name)
				, m_callbacks(callbacks)
			{
				if (bAutoRegisterBuiltinFunctions)
				{

					//
					// register leaf-functions for the <TItem> type (as an extra bonus for whoever registers the item type):
					//
					// - a function for retrieving a global parameter
					// - a function for retrieving the currently being iterated item (this is for usage during the evaluation phase - *not* during generation phase)
					// - an optional function for parsing from a literal value (this one will only be registered if the item can be represented as text)
					// - a function for retrieving the optional item list from a previous query (shuttled items)
					//

					//
					// global param
					//

					{
						stack_string functionName("_builtin_Func_GlobalParam_");
						functionName.append(name);
						static const CFunctionFactory<CFunc_GlobalParam<TItem>> gs_functionFactory_globalParam(functionName.c_str());
					}

					//
					// iterated item
					//

					{
						stack_string functionName("_builtin_Func_IteratedItem_");
						functionName.append(name);
						static const CFunctionFactory<CFunc_IteratedItem<TItem>> gs_functionFactory_iteratedItem(functionName.c_str());
					}

					//
					// literal (only if the item type can be serialized)
					//

					if (callbacks.pSerialize != nullptr)
					{ 
						stack_string functionName("_builtin_Func_Literal_");
						functionName.append(name);
						static const CFunctionFactory<CFunc_Literal<TItem>> gs_functionFactory_literal(functionName.c_str());
					}

					//
					// shuttled items
					//

					{
						stack_string functionName("_builtin_Func_ShuttledItems_");
						functionName.append(name);
						static const CFunctionFactory<CFunc_ShuttledItems<TItem>> gs_functionFactory_shuttledItems(functionName.c_str());
					}
				}
			}

			template <class TItem>
			CItemFactoryInternal<TItem>::~CItemFactoryInternal()
			{
				while (s_pFreeListHoldingSingleItems)
				{
					SHeader* pNext = s_pFreeListHoldingSingleItems->pNextInFreeListOfSingleItems;
					::operator delete(s_pFreeListHoldingSingleItems);
					s_pFreeListHoldingSingleItems = pNext;
				}
			}

			template <class TItem>
			size_t CItemFactoryInternal<TItem>::ComputeMemoryRequiredForSingleItem()
			{
				return sizeof(SHeader);    // the header comes with space for one item already
			}

			template <class TItem>
			size_t CItemFactoryInternal<TItem>::ComputeMemoryRequired(size_t itemCount)
			{
				if (itemCount > 0)
					--itemCount;	// there's already space for the first item in SHeader
				return sizeof(SHeader) + itemCount * sizeof(TItem);
			}

			template <class TItem>
			TItem* CItemFactoryInternal<TItem>::AllocateUninitializedMemoryForItems(size_t numItems)
			{
				if (numItems == 1)
				{
					if (!s_pFreeListHoldingSingleItems)
					{
						const size_t memoryRequired = ComputeMemoryRequiredForSingleItem();
						void* pRawMemory = ::operator new(memoryRequired);
						s_pFreeListHoldingSingleItems = static_cast<SHeader*>(pRawMemory);
						s_pFreeListHoldingSingleItems->itemCount = 1;
						s_pFreeListHoldingSingleItems->pNextInFreeListOfSingleItems = nullptr;
					}

					SHeader* pHeader = s_pFreeListHoldingSingleItems;
					s_pFreeListHoldingSingleItems = s_pFreeListHoldingSingleItems->pNextInFreeListOfSingleItems;
					return pHeader->items;
				}
				else
				{
					// even numItems == 0 is implicitly supported
					const size_t memoryRequired = ComputeMemoryRequired(numItems);
					void* pRawMemory = ::operator new(memoryRequired);
					SHeader* pHeader = static_cast<SHeader*>(pRawMemory);
					pHeader->itemCount = numItems;
					pHeader->pNextInFreeListOfSingleItems = nullptr;
					return pHeader->items;
				}
			}

			template <class TItem>
			void* CItemFactoryInternal<TItem>::CreateItems(size_t numItems, EItemInitMode itemInitMode)
			{
				// - creating 0 items is also supported, but we need to be super-careful to not pass that pointer to CloneItem() then
				// - if it still happens, then CloneItem() would use the uninitialized memory from the header's first item to copy-construct the clone, and this is when undefined behavior enters the scene (!)

				if (itemInitMode == IItemFactory::EItemInitMode::UseUserProvidedFunction && m_callbacks.pCreateDefaultObject == nullptr)
				{
					itemInitMode = IItemFactory::EItemInitMode::UseDefaultConstructor;
				}

				TItem* pFirstItem = AllocateUninitializedMemoryForItems(numItems);
				TItem* pItemWalker = pFirstItem;
				switch (itemInitMode)
				{
				case EItemInitMode::UseDefaultConstructor:
					{
						for (size_t i = 0; i < numItems; ++i, ++pItemWalker)
						{
							new (pItemWalker) TItem;
						}
					}
					break;

				case IItemFactory::EItemInitMode::UseUserProvidedFunction:
					{
						CRY_ASSERT(m_callbacks.pCreateDefaultObject != nullptr);

						TItem temp = (*m_callbacks.pCreateDefaultObject)();
						for (size_t i = 0; i < numItems; ++i, ++pItemWalker)
						{
							new (pItemWalker) TItem(temp);
						}

					}
					break;

				default:
					break;
				}
				
				return pFirstItem;
			}

			template <class TItem>
			void* CItemFactoryInternal<TItem>::CloneItem(const void* pOriginalItem)
			{
				const TItem* pOriginal = static_cast<const TItem*>(pOriginalItem);
				TItem* pClone = AllocateUninitializedMemoryForItems(1);
				new (pClone) TItem(*pOriginal);
				return pClone;
			}

			template <class TItem>
			void CItemFactoryInternal<TItem>::DestroyItems(void* pItems)
			{
				if (pItems)
				{
					void* pRawMemory = static_cast<char*>(pItems) - s_itemsOffsetInHeader;
					SHeader* pHeader = static_cast<SHeader*>(pRawMemory);

					const size_t itemCount = pHeader->itemCount;
					TItem* pItemWalker = pHeader->items;
					for (size_t i = 0; i < itemCount; ++i, ++pItemWalker)
					{
						pItemWalker->~TItem();
						//memset(pItemWalker, 0x1d, sizeof *pItemWalker);
					}

					if (itemCount == 1)
					{
						pHeader->pNextInFreeListOfSingleItems = s_pFreeListHoldingSingleItems;
						s_pFreeListHoldingSingleItems = pHeader;
					}
					else
					{
						::operator delete(pRawMemory);
					}
				}
			}

			template <class TItem>
			const shared::CTypeInfo& CItemFactoryInternal<TItem>::GetItemType() const
			{
				return shared::SDataTypeHelper<TItem>::GetTypeInfo();
			}

			template <class TItem>
			size_t CItemFactoryInternal<TItem>::GetItemSize() const
			{
				return sizeof(TItem);
			}

			template <class TItem>
			void CItemFactoryInternal<TItem>::CopyItem(void* pTargetItem, const void* pSourceItem) const
			{
				TItem& targetItem = *static_cast<TItem*>(pTargetItem);
				const TItem& sourceItem = *static_cast<const TItem*>(pSourceItem);
				targetItem = sourceItem;
			}

			template <class TItem>
			void* CItemFactoryInternal<TItem>::GetItemAtIndex(void* pItems, size_t index) const
			{
				assert(pItems);
				return static_cast<TItem*>(pItems) + index;
			}

			template <class TItem>
			const void* CItemFactoryInternal<TItem>::GetItemAtIndex(const void* pItems, size_t index) const
			{
				assert(pItems);
				return static_cast<const TItem*>(pItems) + index;
			}

			template <class TItem>
			bool CItemFactoryInternal<TItem>::CanBePersistantlySerialized() const
			{
				return (m_callbacks.pSerialize != nullptr);
			}

			template <class TItem>
			bool CItemFactoryInternal<TItem>::TryDeserializeItemIntoDict(shared::IVariantDict& out, const char* szKey, Serialization::IArchive& archive, const char* szName, const char* szLabel)
			{
				assert(m_callbacks.pSerialize != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (m_callbacks.pSerialize == nullptr)
				{
					return false;
				}

				void* pSingleItem = CreateItems(1, EItemInitMode::UseDefaultConstructor);
				if (TryDeserializeItem(pSingleItem, archive, szName, szLabel))
				{
					out.__AddOrReplace(szKey, *this, pSingleItem);
					return true;
				}
				else
				{
					DestroyItems(pSingleItem);
					return false;
				}
			}

			template <class TItem>
			bool CItemFactoryInternal<TItem>::TrySerializeItem(const void* pItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const
			{
				assert(m_callbacks.pSerialize != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (m_callbacks.pSerialize == nullptr)
				{
					return false;
				}

				assert(archive.isOutput());
				if (!archive.isOutput())
				{
					return false;
				}

				// TODO: yasli always wants non-const items - maybe clone the item to be extra safe? Not efficient, though.
				TItem* pItemToSerialize = static_cast<TItem*>(const_cast<void*>(pItem));

				return (*m_callbacks.pSerialize)(archive, *pItemToSerialize, szName, szLabel);
			}

			template <class TItem>
			bool CItemFactoryInternal<TItem>::TryDeserializeItem(void* pOutItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const
			{
				assert(m_callbacks.pSerialize != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (m_callbacks.pSerialize == nullptr)
				{
					return false;
				}

				assert(archive.isInput());
				if (!archive.isInput())
				{
					return false;
				}

				return (*m_callbacks.pSerialize)(archive, *static_cast<TItem*>(pOutItem), szName, szLabel);
			}
			
			template <class TItem>
			void CItemFactoryInternal<TItem>::AddItemToDebugRenderWorld(const void* pItem, core::IDebugRenderWorldPersistent& debugRW) const
			{
				if (m_callbacks.pAddItemToDebugRenderWorld != nullptr)
				{
					(*m_callbacks.pAddItemToDebugRenderWorld)(*static_cast<const TItem*>(pItem), debugRW);
				}
			}

			template <class TItem>
			bool CItemFactoryInternal<TItem>::CanBeRepresentedInDebugRenderWorld() const
			{
				return (m_callbacks.pAddItemToDebugRenderWorld != nullptr);
			}

			template <class TItem>
			void CItemFactoryInternal<TItem>::CreateItemDebugProxyForItem(const void* pItem, core::IItemDebugProxyFactory& itemDebugProxyFactory) const
			{
				if (m_callbacks.pCreateItemDebugProxy != nullptr)
				{
					(*m_callbacks.pCreateItemDebugProxy)(*static_cast<const TItem*>(pItem), itemDebugProxyFactory);
				}
			}

		} // namespace internal

		//===================================================================================
		//
		// CItemFactory<>
		//
		//===================================================================================

		template <class TItem>
		class CItemFactory
		{
		public:
			explicit CItemFactory(const char* name, const SItemFactoryCallbacks<TItem>& callbacks)
			{
				//
				// register the actual item type the caller intends to register
				//

				static const internal::CItemFactoryInternal<TItem> gs_itemFactory(name, callbacks, true);

				//
				// - register a very specific container-type that holds items (plural!) of what the caller just registered
				// - this is for ELeafFunctionKind::ShuttledItems functions to give access to the items that were carried over from one query to another
				// - relates to CFunc_ShuttledItems<> and SContainedTypeRetriever<>
				//

				stack_string itemNameForShuttledItemsContainer("_builtin_ItemFactoryForShuttledItemsContainer_");
				itemNameForShuttledItemsContainer.append(name);
				SItemFactoryCallbacks<CItemListProxy_Readable<TItem>> callbacksForShuttledItemsContainer;  // no callbacks actually
				static const internal::CItemFactoryInternal<CItemListProxy_Readable<TItem>> gs_itemFactoryForContainer(itemNameForShuttledItemsContainer.c_str(), callbacksForShuttledItemsContainer, false);
			}
		};

	}
}
