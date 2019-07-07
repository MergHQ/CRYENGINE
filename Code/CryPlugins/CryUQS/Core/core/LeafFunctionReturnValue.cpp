// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		CLeafFunctionReturnValue::CLeafFunctionReturnValue()
			: m_leafFunctionKind(Client::IFunctionFactory::ELeafFunctionKind::None)
		{
			memset(&m_returnValue, 0, sizeof m_returnValue);  // just a debugging aid
		}

		CLeafFunctionReturnValue::CLeafFunctionReturnValue(const CLeafFunctionReturnValue& rhs)
			: m_leafFunctionKind(Client::IFunctionFactory::ELeafFunctionKind::None)
		{
			memset(&m_returnValue, 0, sizeof m_returnValue);  // just a debugging aid
			CopyOtherToSelf(rhs);
		}

		CLeafFunctionReturnValue::~CLeafFunctionReturnValue()
		{
			Clear();
		}

		CLeafFunctionReturnValue& CLeafFunctionReturnValue::operator=(const CLeafFunctionReturnValue& rhs)
		{
			if (&rhs != this)
			{
				Clear();
				CopyOtherToSelf(rhs);
			}
			return *this;
		}

		void CLeafFunctionReturnValue::SetLiteral(Client::IItemFactory& itemFactory, const void* pItemToClone)
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::None);

			m_returnValue.literal.pItemFactory = &itemFactory;
			m_returnValue.literal.pValue = itemFactory.CloneItem(pItemToClone);

			m_leafFunctionKind = Client::IFunctionFactory::ELeafFunctionKind::Literal;
		}

		void CLeafFunctionReturnValue::SetGlobalParam(const char* szNameOfGlobalParam)
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::None);

			m_returnValue.globalParam.pNameOfGlobalParam = new Shared::CUqsString(szNameOfGlobalParam);

			m_leafFunctionKind = Client::IFunctionFactory::ELeafFunctionKind::GlobalParam;
		}

		void CLeafFunctionReturnValue::SetItemIteration()
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::None);
			m_leafFunctionKind = Client::IFunctionFactory::ELeafFunctionKind::IteratedItem;
		}

		void CLeafFunctionReturnValue::SetShuttledItems()
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::None);
			m_leafFunctionKind = Client::IFunctionFactory::ELeafFunctionKind::ShuttledItems;
		}

		bool CLeafFunctionReturnValue::IsActuallyALeafFunction() const
		{
			return (m_leafFunctionKind != Client::IFunctionFactory::ELeafFunctionKind::None);
		}

		CLeafFunctionReturnValue::SLiteralInfo CLeafFunctionReturnValue::GetLiteral(const SQueryContext& queryContext) const
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::Literal);

			const Shared::CTypeInfo& type = m_returnValue.literal.pItemFactory->GetItemType();
			const void* pValue = m_returnValue.literal.pValue;

			return SLiteralInfo(type, pValue);
		}

		CLeafFunctionReturnValue::SGlobalParamInfo CLeafFunctionReturnValue::GetGlobalParam(const SQueryContext& queryContext) const
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::GlobalParam);

			Client::IItemFactory* pItemFactory = nullptr;
			/*const*/ void* pValue = nullptr;
			const char* szNameOfGlobalParam = m_returnValue.globalParam.pNameOfGlobalParam->c_str();
			const bool bGlobalParamExists = queryContext.globalParams.FindItemFactoryAndObject(szNameOfGlobalParam, pItemFactory, pValue);
			const Shared::CTypeInfo* pType = bGlobalParamExists ? &pItemFactory->GetItemType() : nullptr;

			return SGlobalParamInfo(bGlobalParamExists, pType, pValue, szNameOfGlobalParam);
		}

		CLeafFunctionReturnValue::SItemIterationInfo CLeafFunctionReturnValue::GetItemIteration(const SQueryContext& queryContext) const
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::IteratedItem);

			const IItemList* pGeneratedItems = queryContext.pItemIterationContext ? &queryContext.pItemIterationContext->generatedItems : nullptr;

			return SItemIterationInfo(pGeneratedItems);
		}

		CLeafFunctionReturnValue::SShuttledItemsInfo CLeafFunctionReturnValue::GetShuttledItems(const SQueryContext& queryContext) const
		{
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::ShuttledItems);

			return SShuttledItemsInfo(queryContext.pShuttledItems);  // might be a nullptr, which is OK here (it means that there was no previous query in the chain that put its result-set into the QueryContext)
		}

		void CLeafFunctionReturnValue::Clear()
		{
			switch (m_leafFunctionKind)
			{
			case Client::IFunctionFactory::ELeafFunctionKind::None:
				// nothing
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::Literal:
				m_returnValue.literal.pItemFactory->DestroyItems(m_returnValue.literal.pValue);
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::GlobalParam:
				delete m_returnValue.globalParam.pNameOfGlobalParam;
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::IteratedItem:
				// nothing
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::ShuttledItems:
				// nothing
				break;

			default:
				CRY_ASSERT(0);
			}

			memset(&m_returnValue, 0, sizeof m_returnValue);  // not necessary, but may help pinpoint bugs easier

			m_leafFunctionKind = Client::IFunctionFactory::ELeafFunctionKind::None;
		}

		void CLeafFunctionReturnValue::CopyOtherToSelf(const CLeafFunctionReturnValue& other)
		{
			CRY_ASSERT(&other != this);
			CRY_ASSERT(m_leafFunctionKind == Client::IFunctionFactory::ELeafFunctionKind::None); // should always Clear() beforehand

			switch (other.m_leafFunctionKind)
			{
			case Client::IFunctionFactory::ELeafFunctionKind::None:
				// nothing
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::Literal:
				m_returnValue.literal.pItemFactory = other.m_returnValue.literal.pItemFactory;
				m_returnValue.literal.pValue = other.m_returnValue.literal.pItemFactory->CloneItem(other.m_returnValue.literal.pValue);
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::GlobalParam:
				m_returnValue.globalParam.pNameOfGlobalParam = new Shared::CUqsString(other.m_returnValue.globalParam.pNameOfGlobalParam->c_str());
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::IteratedItem:
				// nothing
				break;

			case Client::IFunctionFactory::ELeafFunctionKind::ShuttledItems:
				// nothing
				break;

			default:
				CRY_ASSERT(0);
			}

			m_leafFunctionKind = other.m_leafFunctionKind;
		}

	}
}
