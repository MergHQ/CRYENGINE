// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CQuery_SequentialBase
		//
		// - base class for queries that have children and run them one after another in sequential order
		// - each time a child query finishes (without exception) the derived class will be asked what to do with it
		// - basically, this class is only coordinating child queries, thus no time budget is required to do some complex (or ongoing) execution
		//
		//===================================================================================

		class CQuery_SequentialBase : public CQueryBase
		{
		protected:
			explicit                                 CQuery_SequentialBase(const SCtorContext& ctorContext);
			                                         ~CQuery_SequentialBase();
			bool                                     HasMoreChildrenLeftToInstantiate() const;
			void                                     StoreResultSetForUseInNextChildQuery(const IQueryResultSet& resultSetOfPreviousChildQuery);
			void                                     InstantiateNextChildQueryBlueprint();

		private:
			// CQueryBase
			virtual bool                             OnInstantiateFromQueryBlueprint(const shared::IVariantDict& runtimeParams, shared::CUqsString& error) override final;
			virtual EUpdateState                     OnUpdate(shared::CUqsString& error) override final;
			virtual void                             OnCancel() override final;
			virtual void                             OnGetStatistics(SStatistics& out) const override final;
			// ~CQueryBase

			void                                     OnChildQueryFinished(const SQueryResult& result);
			virtual void                             HandleChildQueryFinishedWithSuccess(const CQueryID& childQueryID, QueryResultSetUniquePtr&& pResultSet) = 0;

		private:
			shared::CVariantDict                     m_runtimeParams;
			size_t                                   m_indexOfNextChildToInstantiate;
			CQueryID                                 m_queryIDOfCurrentlyRunningChild;
			std::unique_ptr<CItemList>               m_pResultingItemsOfLastChildQuery;
			bool                                     m_bExceptionOccurredInChild;
			shared::CUqsString                       m_exceptionMessageFromChild;
		};

	}
}
