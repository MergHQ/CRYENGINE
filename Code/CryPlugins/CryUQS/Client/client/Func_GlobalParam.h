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
			// CFunc_GlobalParam<>
			//
			//===================================================================================

			template <class TGlobalParam>
			class CFunc_GlobalParam final : public CFunctionBase<CFunc_GlobalParam<TGlobalParam>, TGlobalParam, IFunctionFactory::ELeafFunctionKind::GlobalParam>
			{
			private:
				// these 4 typedefs exist only to make gcc happy
				typedef CFunctionBase<CFunc_GlobalParam<TGlobalParam>, TGlobalParam, IFunctionFactory::ELeafFunctionKind::GlobalParam> BaseClass;
				typedef typename BaseClass::SCtorContext SCtorContext;
				typedef typename BaseClass::SExecuteContext SExecuteContext;
				typedef typename BaseClass::SValidationContext SValidationContext;

			public:
				explicit                      CFunc_GlobalParam(const SCtorContext& ctorContext);
				TGlobalParam                  DoExecute(const SExecuteContext& executeContext) const;

			private:
				virtual bool                  ValidateDynamic(const SValidationContext& validationContext) const override;

			private:
				string                        m_nameOfGlobalParam;               // for validation error message
				IItemFactory*                 m_pItemFactoryOfGlobalParam;
				void*                         m_pValueOfGlobalParam;
			};

			template <class TGlobalParam>
			CFunc_GlobalParam<TGlobalParam>::CFunc_GlobalParam(const SCtorContext& ctorContext)
				: BaseClass(ctorContext)
				, m_nameOfGlobalParam(ctorContext.optionalReturnValueForLeafFunctions)
				, m_pItemFactoryOfGlobalParam(nullptr)
				, m_pValueOfGlobalParam(nullptr)
			{
				ctorContext.blackboard.globalParams.FindItemFactoryAndObject(m_nameOfGlobalParam.c_str(), m_pItemFactoryOfGlobalParam, m_pValueOfGlobalParam);
				assert((m_pItemFactoryOfGlobalParam && m_pValueOfGlobalParam) || (!m_pItemFactoryOfGlobalParam && !m_pValueOfGlobalParam));
			}

			template <class TGlobalParam>
			bool CFunc_GlobalParam<TGlobalParam>::ValidateDynamic(const SValidationContext& validationContext) const
			{
				if (m_pItemFactoryOfGlobalParam)
				{
					const shared::CTypeInfo& expectedType = shared::SDataTypeHelper<TGlobalParam>::GetTypeInfo();
					const shared::CTypeInfo& receivedType = m_pItemFactoryOfGlobalParam->GetItemType();

					if (expectedType == receivedType)
					{
						return true;
					}
					else
					{
						validationContext.error.Format("%s: global param '%s' exists, but mismatches the type (expected a '%s', but actually stored is a '%s')",
							validationContext.nameOfFunctionBeingValidated, m_nameOfGlobalParam.c_str(), expectedType.name(), receivedType.name());
						return false;
					}
				}
				else
				{
					validationContext.error.Format("%s: global param '%s' does not exist", validationContext.nameOfFunctionBeingValidated, m_nameOfGlobalParam.c_str());
					return false;
				}
			}

			template <class TGlobalParam>
			TGlobalParam CFunc_GlobalParam<TGlobalParam>::DoExecute(const SExecuteContext& executeContext) const
			{
				assert(m_pItemFactoryOfGlobalParam);	// cannot fail if the validation succeeded (presuming the caller did not cheat)
				return *static_cast<const TGlobalParam*>(m_pValueOfGlobalParam);
			}

		}
	}
}
