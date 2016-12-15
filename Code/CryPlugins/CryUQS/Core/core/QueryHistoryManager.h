// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CQueryHistoryManager
		//
		//===================================================================================

		class CQueryHistoryManager : public IQueryHistoryManager
		{
		public:
			explicit                           CQueryHistoryManager();

			// IQueryHistoryManager
			virtual void                       RegisterQueryHistoryListener(IQueryHistoryListener* pListener) override;
			virtual void                       UnregisterQueryHistoryListener(IQueryHistoryListener* pListener) override;
			virtual void                       UpdateDebugRendering3D(const SDebugCameraView& view, const SEvaluatorDrawMasks& evaluatorDrawMasks) override;
			virtual bool                       SerializeLiveQueryHistory(const char* xmlFilePath, shared::IUqsString& error) override;
			virtual bool                       DeserializeQueryHistory(const char* xmlFilePath, shared::IUqsString& error) override;
			virtual void                       MakeQueryHistoryCurrent(EHistoryOrigin whichHistory) override;
			virtual EHistoryOrigin             GetCurrentQueryHistory() const override;
			virtual void                       ClearQueryHistory(EHistoryOrigin whichHistory) override;
			virtual void                       EnumerateHistoricQueries(EHistoryOrigin whichHistory, IQueryHistoryConsumer& receiver) const override;
			virtual void                       MakeHistoricQueryCurrentForInWorldRendering(EHistoryOrigin whichHistory, const CQueryID& queryIDToMakeCurrent) override;
			virtual void                       EnumerateInstantEvaluatorNames(EHistoryOrigin fromWhichHistory, const CQueryID& idOfQueryUsingTheseEvaluators, IQueryHistoryConsumer& receiver) override;
			virtual void                       EnumerateDeferredEvaluatorNames(EHistoryOrigin fromWhichHistory, const CQueryID& idOfQueryUsingTheseEvaluators, IQueryHistoryConsumer& receiver) override;
			virtual CQueryID                   GetCurrentHistoricQueryForInWorldRendering(EHistoryOrigin fromWhichHistory) const override;
			virtual void                       GetDetailsOfHistoricQuery(EHistoryOrigin whichHistory, const CQueryID& queryIDToGetDetailsFor, IQueryHistoryConsumer& receiver) const override;
			virtual void                       GetDetailsOfFocusedItem(IQueryHistoryConsumer& receiver) const override;
			virtual size_t                     GetRoughMemoryUsageOfQueryHistory(EHistoryOrigin whichHistory) const override;
			virtual size_t                     GetHistoricQueriesCount(EHistoryOrigin whichHistory) const override;
			// ~IQueryHistoryManager

			HistoricQuerySharedPtr             AddNewLiveHistoricQuery(const CQueryID& queryID, const char* querierName, const CQueryID& parentQueryID);
			void                               UnderlyingQueryIsGettingDestroyed(const CQueryID& queryID);

		private:
			                                   UQS_NON_COPYABLE(CQueryHistoryManager);

			void                               NotifyListeners(IQueryHistoryListener::EEvent ev) const;

		private:
			CQueryHistory                      m_queryHistories[2];                  // "live" and "deserialized" query history; the live one gets filled by ongoing queries from the CQueryManager
			CQueryID                           m_queryIDOfCurrentHistoricQuery[2];   // ditto
			EHistoryOrigin                     m_historyToManage;                    // used as index into m_queryHistories[] and m_queryIDOfCurrentHistoricQuery[]
			size_t                             m_indexOfFocusedItem;
			std::list<IQueryHistoryListener*>  m_listeners;

			static const size_t                s_noItemFocusedIndex = (size_t)-1;
		};

	}
}
