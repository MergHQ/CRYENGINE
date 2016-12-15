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
			// CFunc_Literal<>
			//
			//===================================================================================

			template <class TLiteral>
			class CFunc_Literal final : public CFunctionBase<CFunc_Literal<TLiteral>, TLiteral, IFunctionFactory::ELeafFunctionKind::Literal>
			{
			private:
				// these 4 typedefs exist only to make gcc happy
				typedef CFunctionBase<CFunc_Literal<TLiteral>, TLiteral, IFunctionFactory::ELeafFunctionKind::Literal> BaseClass;
				typedef typename BaseClass::SCtorContext SCtorContext;
				typedef typename BaseClass::SExecuteContext SExecuteContext;
				typedef typename BaseClass::SValidationContext SValidationContext;

			public:
				explicit                 CFunc_Literal(const SCtorContext& ctorContext);
				TLiteral                 DoExecute(const SExecuteContext& executeContext) const;

			private:
				virtual bool             ValidateDynamic(const SValidationContext& validationContext) const override;

			private:
				string                   m_literalAsString;       // for validation error message
				TLiteral                 m_literal;
				bool                     m_parsedSuccessfully;
			};

			template <class TLiteral>
			CFunc_Literal<TLiteral>::CFunc_Literal(const SCtorContext& ctorContext)
				: BaseClass(ctorContext)
				, m_literalAsString(ctorContext.optionalReturnValueForLeafFunctions)
				, m_parsedSuccessfully(false)
			{
				const IItemFactory* pItemFactory = BaseClass::GetItemFactoryOfReturnType(); // need "BaseClass::" (or "this->") to make it a dependant expression, otherwise gcc/orbis will complain about use of undeclared identifier
				assert(pItemFactory != nullptr);
				if (pItemFactory != nullptr)
				{
					core::IItemSerializationSupport& itemSerializationSupport = core::IHubPlugin::GetHub().GetItemSerializationSupport();
					shared::IUqsString* pErrorMessage = nullptr;
					m_parsedSuccessfully = itemSerializationSupport.DeserializeItemFromCStringLiteral(&m_literal, *pItemFactory, m_literalAsString.c_str(), pErrorMessage);
				}
			}

			template <class TLiteral>
			bool CFunc_Literal<TLiteral>::ValidateDynamic(const SValidationContext& validationContext) const
			{
				if (m_parsedSuccessfully)
				{
					return true;
				}
				else
				{
					validationContext.error.Format("%s: could not parse '%s' into a %s", validationContext.nameOfFunctionBeingValidated, m_literalAsString.c_str(), shared::SDataTypeHelper<TLiteral>::GetTypeInfo().name());
					return false;
				}
			}

			template <class TLiteral>
			TLiteral CFunc_Literal<TLiteral>::DoExecute(const SExecuteContext& executeContext) const
			{
				assert(m_parsedSuccessfully);
				return m_literal;
			}

		}
	}
}
