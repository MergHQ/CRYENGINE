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
			// CFunc_ShuttledItems<>
			//
			//===================================================================================

			template <class TItem>
			class CFunc_ShuttledItems final : public CFunctionBase<CFunc_ShuttledItems<TItem>, CItemListProxy_Readable<TItem>, IFunctionFactory::ELeafFunctionKind::ShuttledItems>
			{
			private:
				// these 4 typedefs exist only to make gcc happy
				typedef CFunctionBase<CFunc_ShuttledItems<TItem>, CItemListProxy_Readable<TItem>, IFunctionFactory::ELeafFunctionKind::ShuttledItems> BaseClass;
				typedef typename BaseClass::SCtorContext SCtorContext;
				typedef typename BaseClass::SExecuteContext SExecuteContext;
				typedef typename BaseClass::SValidationContext SValidationContext;

			public:
				explicit                                         CFunc_ShuttledItems(const SCtorContext& ctorContext);
				~CFunc_ShuttledItems();
				CItemListProxy_Readable<TItem>                   DoExecute(const SExecuteContext& executeContext) const;

			private:
				virtual bool                                     ValidateDynamic(const SValidationContext& validationContext) const override;

			private:
				CItemListProxy_Readable<TItem>*                  m_pItemListProxy;
				string                                           m_errorMessage;
			};

			template <class TItem>
			CFunc_ShuttledItems<TItem>::CFunc_ShuttledItems(const SCtorContext& ctorContext)
				: BaseClass(ctorContext)
				, m_pItemListProxy(nullptr)
			{
				// check for existence of shuttled items
				if (ctorContext.blackboard.pShuttledItems)
				{
					// check for correct type of shuttled items
					const shared::CTypeInfo& typeOfShuttledItems = ctorContext.blackboard.pShuttledItems->GetItemFactory().GetItemType();
					const shared::CTypeInfo& expectedType = shared::SDataTypeHelper<TItem>::GetTypeInfo();
					if (typeOfShuttledItems == expectedType)
					{
						m_pItemListProxy = new CItemListProxy_Readable<TItem>(*ctorContext.blackboard.pShuttledItems);
					}
					else
					{
						m_errorMessage.Format("expected the shuttled items to be of type '%s', but they are '%s'", expectedType.name(), typeOfShuttledItems.name());
					}
				}
				else
				{
					m_errorMessage = "there are no shuttled items from a potential previous query on the blackboard";
				}
			}

			template <class TItem>
			CFunc_ShuttledItems<TItem>::~CFunc_ShuttledItems()
			{
				delete m_pItemListProxy;
			}

			template <class TItem>
			bool CFunc_ShuttledItems<TItem>::ValidateDynamic(const SValidationContext& validationContext) const
			{
				if (m_pItemListProxy)
				{
					return true;
				}
				else
				{
					validationContext.error.Format("%s: %s", validationContext.nameOfFunctionBeingValidated, m_errorMessage.c_str());
					return false;
				}
			}

			template <class TItem>
			CItemListProxy_Readable<TItem> CFunc_ShuttledItems<TItem>::DoExecute(const SExecuteContext& executeContext) const
			{
				assert(m_pItemListProxy);  // should have been caught by DoValidateDynamic() already
				return *m_pItemListProxy;
			}

		}
	}
}
