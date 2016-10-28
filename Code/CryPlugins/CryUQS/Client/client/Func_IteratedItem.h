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
			// CFunc_IteratedItem<>
			//
			//===================================================================================

			template <class TItem>
			class CFunc_IteratedItem final : public CFunctionBase<CFunc_IteratedItem<TItem>, TItem, IFunctionFactory::ELeafFunctionKind::IteratedItem>
			{
			private:
				// these 4 typedefs exist only to make gcc happy
				typedef CFunctionBase<CFunc_IteratedItem<TItem>, TItem, IFunctionFactory::ELeafFunctionKind::IteratedItem> BaseClass;
				typedef typename BaseClass::SCtorContext SCtorContext;
				typedef typename BaseClass::SExecuteContext SExecuteContext;
				typedef typename BaseClass::SValidationContext SValidationContext;

			public:
				explicit                                    CFunc_IteratedItem(const SCtorContext& ctorContext);
				TItem                                       DoExecute(const SExecuteContext& executeContext) const;

			private:
				virtual bool                                ValidateDynamic(const SValidationContext& validationContext) const override;

			private:
				const core::SItemIterationContext*          m_pItemIterationContext;   // FIXME: weak pointer?
				bool                                        m_itemTypeIsFine;
			};

			template <class TItem>
			CFunc_IteratedItem<TItem>::CFunc_IteratedItem(const SCtorContext& ctorContext)
				: BaseClass(ctorContext)
				, m_pItemIterationContext(ctorContext.blackboard.pItemIterationContext)
				, m_itemTypeIsFine(false)
			{
				if (m_pItemIterationContext)
				{
					m_itemTypeIsFine = (m_pItemIterationContext->generatedItems.GetItemFactory().GetItemType() == shared::SDataTypeHelper<TItem>::GetTypeInfo());
				}
			}

			template <class TItem>
			bool CFunc_IteratedItem<TItem>::ValidateDynamic(const SValidationContext& validationContext) const
			{
				if (m_pItemIterationContext)
				{
					if (m_itemTypeIsFine)
					{
						return true;
					}
					else
					{
						validationContext.error.Format("%s: item type mismatch: this function expects items of type '%s', but the underlying item list contains items of type '%s'",
							validationContext.nameOfFunctionBeingValidated, shared::SDataTypeHelper<TItem>::GetTypeInfo().name(), m_pItemIterationContext->generatedItems.GetItemFactory().GetItemType().name());
						return false;
					}
				}
				else
				{
					validationContext.error.Format("%s: we're currently not iterating on items (we're most likely still in the Generator phase and this function should only be used in the Evaluator phase)",
						validationContext.nameOfFunctionBeingValidated);
					return false;
				}
			}

			template <class TItem>
			TItem CFunc_IteratedItem<TItem>::DoExecute(const SExecuteContext& executeContext) const
			{
				assert(m_pItemIterationContext);
				assert(m_pItemIterationContext->generatedItems.GetItemFactory().GetItemType() == shared::SDataTypeHelper<TItem>::GetTypeInfo());
				const client::CItemListProxy_Readable<TItem> actualItems(m_pItemIterationContext->generatedItems);
				return actualItems.GetItemAtIndex(executeContext.currentItemIndex);
			}

		}
	}
}
