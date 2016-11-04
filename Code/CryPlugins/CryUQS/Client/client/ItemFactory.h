// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
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
				virtual void                         AddItemToDebugRenderWorld(const void* pItem, core::IDebugRenderWorld& debugRW) const override = 0;
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
			// SItemFactoryCallbackTypes<>
			//
			//===================================================================================

			template <class TItem>
			struct SItemFactoryCallbackTypes
			{
				typedef bool (*serializeFunc_t)(Serialization::IArchive& archive, TItem& item, const char* szName, const char* szLabel);
				typedef TItem (*createDefaultObjectFunc_t)();
				typedef void (*addItemToDebugRenderWorldFunc_t)(const TItem& item, core::IDebugRenderWorld& debugRW);
				typedef void (*createItemDebugProxyFunc_t)(const TItem& item, core::IItemDebugProxyFactory& itemDebugProxyFactory);
			};

			//===================================================================================
			//
			// CItemFactoryInternal<>
			//
			// - passing a nullptr tSerializeFunc is OK and means that the item can NOT be persistent serialized in textual form
			// - the tAddItemToDebugRenderWorldFunc can be a nullptr or valid pointer (it's used to add one of several debug-primitives to a given IDebugRenderWorld upon function calls)
			// - the tCreateItemDebugProxyFunc can be nullptr or valid pointer (it's used to create a geometrical debug proxy that will be used to pick a generated item for detailed inspection in the live 3D world after the query has finished)
			//
			//===================================================================================

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
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

				explicit                            CItemFactoryInternal(const char* name, typename SItemFactoryCallbackTypes<TItem>::createDefaultObjectFunc_t pCreateDefaultItemFunc, bool bAutoRegisterBuiltinFunctions);
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
				virtual void                        AddItemToDebugRenderWorld(const void* item, core::IDebugRenderWorld& debugRW) const override;
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

				const typename SItemFactoryCallbackTypes<TItem>::createDefaultObjectFunc_t m_pCreateDefaultItemFunc;

				static const ptrdiff_t              s_itemsOffsetInHeader = offsetof(SHeader, items);
				static SHeader*                     s_pFreeListHoldingSingleItems;
			};

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::GetItemAtIndex(void* pItems, size_t index) const
			{
				assert(pItems);
				return static_cast<TItem*>(pItems) + index;
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			const void* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::GetItemAtIndex(const void* pItems, size_t index) const
			{
				assert(pItems);
				return static_cast<const TItem*>(pItems) + index;
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			typename CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::SHeader* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::s_pFreeListHoldingSingleItems;

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CItemFactoryInternal(
				const char* name, 
				typename SItemFactoryCallbackTypes<TItem>::createDefaultObjectFunc_t pCreateDefaultItemFunc, 
				bool bAutoRegisterBuiltinFunctions)
				: CItemFactoryBase(name)
				, m_pCreateDefaultItemFunc(pCreateDefaultItemFunc)
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

					if (tSerializeFunc != nullptr)
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

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::~CItemFactoryInternal()
			{
				while (s_pFreeListHoldingSingleItems)
				{
					SHeader* pNext = s_pFreeListHoldingSingleItems->pNextInFreeListOfSingleItems;
					::operator delete(s_pFreeListHoldingSingleItems);
					s_pFreeListHoldingSingleItems = pNext;
				}
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			size_t CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::ComputeMemoryRequiredForSingleItem()
			{
				return sizeof(SHeader);    // the header comes with space for one item already
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			size_t CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::ComputeMemoryRequired(size_t itemCount)
			{
				if (itemCount > 0)
					--itemCount;	// there's already space for the first item in SHeader
				return sizeof(SHeader) + itemCount * sizeof(TItem);
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			TItem* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::AllocateUninitializedMemoryForItems(size_t numItems)
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

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CreateItems(size_t numItems, EItemInitMode itemInitMode)
			{
				// - creating 0 items is also supported, but we need to be super-careful to not pass that pointer to CloneItem() then
				// - if it still happens, then CloneItem() would use the uninitialized memory from the header's first item to copy-construct the clone, and this is when undefined behavior enters the scene (!)

				if (itemInitMode == IItemFactory::EItemInitMode::UseUserProvidedFunction && (m_pCreateDefaultItemFunc == nullptr))
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
						CRY_ASSERT(m_pCreateDefaultItemFunc != nullptr);

						TItem temp = (*m_pCreateDefaultItemFunc)();
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

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void* CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CloneItem(const void* pOriginalItem)
			{
				const TItem* pOriginal = static_cast<const TItem*>(pOriginalItem);
				TItem* pClone = AllocateUninitializedMemoryForItems(1);
				new (pClone) TItem(*pOriginal);
				return pClone;
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::DestroyItems(void* pItems)
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

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			const shared::CTypeInfo& CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::GetItemType() const
			{
				return shared::SDataTypeHelper<TItem>::GetTypeInfo();
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			size_t CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::GetItemSize() const
			{
				return sizeof(TItem);
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CopyItem(void* pTargetItem, const void* pSourceItem) const
			{
				TItem& targetItem = *static_cast<TItem*>(pTargetItem);
				const TItem& sourceItem = *static_cast<const TItem*>(pSourceItem);
				targetItem = sourceItem;
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
				bool CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CanBePersistantlySerialized() const
			{
				return (tSerializeFunc != nullptr);
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			bool CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::TryDeserializeItemIntoDict(shared::IVariantDict& out, const char* szKey, Serialization::IArchive& archive, const char* szName, const char* szLabel)
			{
				assert(tSerializeFunc != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (tSerializeFunc == nullptr)
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


			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
				bool CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::TrySerializeItem(const void* pItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const
			{
				assert(tSerializeFunc != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (tSerializeFunc == nullptr)
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

				return (*tSerializeFunc)(archive, *pItemToSerialize, szName, szLabel);
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
				bool CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::TryDeserializeItem(void* pOutItem, Serialization::IArchive& archive, const char* szName, const char* szLabel) const
			{
				assert(tSerializeFunc != nullptr);  // callers should check with CanBePersistantlySerialized() beforehand
				if (tSerializeFunc == nullptr)
				{
					return false;
				}

				assert(archive.isInput());
				if (!archive.isInput())
				{
					return false;
				}

				return (*tSerializeFunc)(archive, *static_cast<TItem*>(pOutItem), szName, szLabel);
			}
			
			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::AddItemToDebugRenderWorld(const void* pItem, core::IDebugRenderWorld& debugRW) const
			{
				if (tAddItemToDebugRenderWorldFunc != nullptr)
				{
					(*tAddItemToDebugRenderWorldFunc)(*static_cast<const TItem*>(pItem), debugRW);
				}
			}

			template <
				class TItem,
				typename SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
				typename SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
				typename SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
			>
			void CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc>::CreateItemDebugProxyForItem(const void* pItem, core::IItemDebugProxyFactory& itemDebugProxyFactory) const
			{
				if (tCreateItemDebugProxyFunc != nullptr)
				{
					(*tCreateItemDebugProxyFunc)(*static_cast<const TItem*>(pItem), itemDebugProxyFactory);
				}
			}

		} // namespace internal

		//===================================================================================
		//
		// CItemFactory<>
		//
		//===================================================================================

		template <
			class TItem,
			typename internal::SItemFactoryCallbackTypes<TItem>::serializeFunc_t tSerializeFunc,
			typename internal::SItemFactoryCallbackTypes<TItem>::addItemToDebugRenderWorldFunc_t tAddItemToDebugRenderWorldFunc,
			typename internal::SItemFactoryCallbackTypes<TItem>::createItemDebugProxyFunc_t tCreateItemDebugProxyFunc
		>
		class CItemFactory
		{
		public:
			explicit CItemFactory(const char* name, typename internal::SItemFactoryCallbackTypes<TItem>::createDefaultObjectFunc_t pCreateDefaultItemFunc)
			{
				//
				// register the actual item type the caller intends to register
				//

				static const internal::CItemFactoryInternal<TItem, tSerializeFunc, tAddItemToDebugRenderWorldFunc, tCreateItemDebugProxyFunc> gs_itemFactory(name, pCreateDefaultItemFunc, true);

				//
				// - register a very specific container-type that holds items (plural!) of what the caller just registered
				// - this is for ELeafFunctionKind::ShuttledItems functions to give access to the items that were carried over from one query to another
				// - relates to CFunc_ShuttledItems<> and SContainedTypeRetriever<>
				//

				stack_string itemNameForShuttledItemsContainer("_builtin_ItemFactoryForShuttledItemsContainer_");
				itemNameForShuttledItemsContainer.append(name);
				static const internal::CItemFactoryInternal<CItemListProxy_Readable<TItem>, nullptr, nullptr, nullptr> gs_itemFactoryForContainer(itemNameForShuttledItemsContainer.c_str(), nullptr, false);
			}
		};

	}
}
