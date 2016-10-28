// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// - all query factories that the core provides right now
		// - they need to be defined here as this translation unit is guaranteed to survive even in a .lib (CQueryFactoryBase::RegisterAllInstancesInDatabase() gets called from the central CHub)
		// - if more query types ever get introduced, then an according factory and specialization for CQueryFactory<>::GetQueryBlueprintType() and
		//   CQueryFactory<>::CheckOutputTypeCompatibilityAmongChildQueryBlueprints() need to be added
		//
		//===================================================================================

		static const CQueryFactory<CQuery_Regular> gs_qf_regularQuery("Regular", true, true, true, 0, 0);
		static const CQueryFactory<CQuery_Chained> gs_qf_chainedQuery("Chained", false, false, false, 1, IQueryFactory::kUnlimitedChildren);
		static const CQueryFactory<CQuery_Fallbacks> gs_qf_fallbacksQuery("Fallbacks", false, false, false, 1, IQueryFactory::kUnlimitedChildren);

		//===================================================================================
		//
		// specializations for CQueryFactory<>::GetQueryBlueprintType()
		//
		//===================================================================================

		// specialization for CQuery_Regular: in a regular query, it's the generator of that query that dictates the output type
		template <>
		const shared::CTypeInfo& CQueryFactory<CQuery_Regular>::GetQueryBlueprintType(const CQueryBlueprint& queryBlueprint) const
		{
			const CGeneratorBlueprint* pGeneratorBlueprint = queryBlueprint.GetGeneratorBlueprint();
			assert(pGeneratorBlueprint);  // a CQuery_Regular without a generator-blueprint can never ever be valid
			return pGeneratorBlueprint->GetTypeOfItemsToGenerate();
		}

		// specialization for CQuery_Chained: in a chained query, the last child in the chain dictates the output type
		template <>
		const shared::CTypeInfo& CQueryFactory<CQuery_Chained>::GetQueryBlueprintType(const CQueryBlueprint& queryBlueprint) const
		{
			const size_t childCount = queryBlueprint.GetChildCount();
			assert(childCount > 0);  // a chained query without children can never ever be valid
			std::shared_ptr<const CQueryBlueprint> pLastChildInChain = queryBlueprint.GetChild(childCount - 1);
			return pLastChildInChain->GetOutputType();
		}

		// specialization for CQuery_Fallbacks: in a fallback query, all children are supposed to return the same item type, so it doesn't matter which child dictates the output type (we'll just pick the first one)
		template <>
		const shared::CTypeInfo& CQueryFactory<CQuery_Fallbacks>::GetQueryBlueprintType(const CQueryBlueprint& queryBlueprint) const
		{
			assert(queryBlueprint.GetChildCount() > 0);  // a fallback query without children can never ever be valid
			std::shared_ptr<const CQueryBlueprint> pFirstChild = queryBlueprint.GetChild(0);
			return pFirstChild->GetOutputType();
		}

		//===================================================================================
		//
		// specializations for CQueryFactory<>::CheckOutputTypeCompatibilityAmongChildQueryBlueprints()
		//
		//===================================================================================

		// specialization for CQuery_Regular: since a regular query has no children, no special checks for output compatibility needs to be done
		template <>
		bool CQueryFactory<CQuery_Regular>::CheckOutputTypeCompatibilityAmongChildQueryBlueprints(const CQueryBlueprint& parentQueryBlueprint, string& error, size_t& childIndexCausingTheError) const
		{
			return true;
		}

		// specialization for CQuery_Chained: the output of each child needs to be compatible with the input of its succeeding sibling
		template <>
		bool CQueryFactory<CQuery_Chained>::CheckOutputTypeCompatibilityAmongChildQueryBlueprints(const CQueryBlueprint& parentQueryBlueprint, string& error, size_t& childIndexCausingTheError) const
		{
			const size_t numChildren = parentQueryBlueprint.GetChildCount();
			assert(numChildren > 0);  // a chained query without children can never ever be valid by the time we reach here

			for (size_t i = 0; i < numChildren - 1; ++i)
			{
				std::shared_ptr<const CQueryBlueprint> childA = parentQueryBlueprint.GetChild(i);
				std::shared_ptr<const CQueryBlueprint> childB = parentQueryBlueprint.GetChild(i + 1);

				// grab the expected type of the succeeding child's input
				const shared::CTypeInfo* pInputType = childB->GetExpectedShuttleType();

				if (!pInputType)
				{
					// the succeeding child has not specified a type it expects from the shuttled items, but it doesn't mean it's an error!
					// -> reason: now that we've survived the whole resolving pipeline until here, obviously no other function/whatever complained about the missing
					//    type of shuttled items, which means that no one is making use of shuttled items at all, so we don't need to care here as well
					//    => just continue with the next child
					continue;
				}

				// the output of previous child must match the expected input of the next child
				const shared::CTypeInfo& outputType = childA->GetOutputType();
				if (outputType != *pInputType)
				{
					error.Format("the shuttle type (%s) mismatches the output type of the preceding child (%s)", pInputType->name(), outputType.name());
					childIndexCausingTheError = i;
					return false;
				}
			}

			return true;
		}

		// specialization for CQuery_Fallbacks: all children need to have the same output type
		template <>
		bool CQueryFactory<CQuery_Fallbacks>::CheckOutputTypeCompatibilityAmongChildQueryBlueprints(const CQueryBlueprint& parentQueryBlueprint, string& error, size_t& childIndexCausingTheError) const
		{
			const size_t numChildren = parentQueryBlueprint.GetChildCount();
			assert(numChildren > 0);  // a fallback query without children can never ever be valid by the time we reach here

			for (size_t i = 0; i < numChildren - 1; ++i)
			{
				std::shared_ptr<const CQueryBlueprint> childA = parentQueryBlueprint.GetChild(i);
				std::shared_ptr<const CQueryBlueprint> childB = parentQueryBlueprint.GetChild(i + 1);

				const shared::CTypeInfo& typeA = childA->GetOutputType();
				const shared::CTypeInfo& typeB = childB->GetOutputType();

				if (typeA != typeB)
				{
					error.Format("the output type must be '%s' because the preceding child outputs items of this type", typeA.name());
					childIndexCausingTheError = i + 1;
					return false;
				}
			}

			return true;
		}

		//===================================================================================
		//
		// CQueryFactoryBase
		//
		//===================================================================================

		CQueryFactoryBase* CQueryFactoryBase::s_pList;

		CQueryFactoryBase::CQueryFactoryBase(const char* name, bool bSupportsParameters, bool bRequiresGenerator, bool bSupportsEvaluators, size_t minRequiredChildren, size_t maxAllowedChildren)
			: m_name(name)
			, m_bSupportsParameters(bSupportsParameters)
			, m_bRequiresGenerator(bRequiresGenerator)
			, m_bSupportsEvaluators(bSupportsEvaluators)
			, m_minRequiredChildren(minRequiredChildren)
			, m_maxAllowedChildren(maxAllowedChildren)
			, m_pNext(s_pList)
		{
			s_pList = this;
		}

		const char* CQueryFactoryBase::GetName() const
		{
			return m_name.c_str();
		}

		bool CQueryFactoryBase::SupportsParameters() const
		{
			return m_bSupportsParameters;
		}

		bool CQueryFactoryBase::RequiresGenerator() const
		{
			return m_bRequiresGenerator;
		}

		bool CQueryFactoryBase::SupportsEvaluators() const
		{
			return m_bSupportsEvaluators;
		}

		size_t CQueryFactoryBase::GetMinRequiredChildren() const
		{
			return m_minRequiredChildren;
		}

		size_t CQueryFactoryBase::GetMaxAllowedChildren() const
		{
			return m_maxAllowedChildren;
		}

		void CQueryFactoryBase::RegisterAllInstancesInDatabase(QueryFactoryDatabase& databaseToRegisterIn)
		{
			for (CQueryFactoryBase* pCurrent = s_pList; pCurrent; pCurrent = pCurrent->m_pNext)
			{
				databaseToRegisterIn.RegisterFactory(pCurrent, pCurrent->m_name.c_str());
			}
		}

	}
}
