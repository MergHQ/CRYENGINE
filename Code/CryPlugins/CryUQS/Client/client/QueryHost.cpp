// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// CQueryHost
		//
		//===================================================================================

		CQueryHost::CQueryHost()
			: m_runningStatus(ERunningStatus::HasNotBeenStartedYet)
			, m_pExpectedOutputType(nullptr)
			, m_queryBlueprintID()
			, m_runtimeParams()
			, m_queryID(core::CQueryID::CreateInvalid())
			, m_pResultSet()
		{
			// nothing
		}

		CQueryHost::~CQueryHost()
		{
			// cancel a possibly running query
			if (m_queryID.IsValid())
			{
				if (core::IHub* pHub = core::IHubPlugin::GetHubPtr())
				{
					pHub->GetQueryManager().CancelQuery(m_queryID);
				}
			}
		}

		void CQueryHost::SetExpectedOutputType(const shared::CTypeInfo* pExpectedOutputType)
		{
			m_pExpectedOutputType = pExpectedOutputType;
		}

		void CQueryHost::SetQueryBlueprint(const char* szQueryBlueprintName)
		{
			if (core::IHub* pHub = core::IHubPlugin::GetHubPtr())
			{
				m_queryBlueprintID = pHub->GetQueryBlueprintLibrary().FindQueryBlueprintIDByName(szQueryBlueprintName);
			}

			m_queryBlueprintName = szQueryBlueprintName;
		}

		const char* CQueryHost::GetQueryBlueprintName() const
		{
			return m_queryBlueprintName.c_str();
		}

		void CQueryHost::SetQueryBlueprint(const core::CQueryBlueprintID& queryBlueprintID)
		{
			m_queryBlueprintID = queryBlueprintID;
			m_queryBlueprintName = queryBlueprintID.GetQueryBlueprintName();
		}

		void CQueryHost::SetQuerierName(const char* szQuerierName)
		{
			m_querierName = szQuerierName;
		}

		shared::CVariantDict& CQueryHost::GetRuntimeParamsStorage()
		{
			return m_runtimeParams;
		}

		void CQueryHost::StartQuery()
		{
			HelpStartQuery();

			// definitely clear the runtime-params for next start
			m_runtimeParams.Clear();
		}

		void CQueryHost::HelpStartQuery()
		{
			core::IHub* pHub = core::IHubPlugin::GetHubPtr();

			//
			// get rid of the previous result set (if not yet done via ClearResultSet())
			//

			m_pResultSet.reset();

			//
			// cancel a possibly running query
			//

			if (m_queryID.IsValid())
			{
				if (pHub)
				{
					pHub->GetQueryManager().CancelQuery(m_queryID);
				}

				m_queryID = core::CQueryID::CreateInvalid();
			}

			//
			// ensure the UQS plugin is present
			//

			if (!pHub)
			{
				m_exceptionMessageIfAny = "UQS Plugin is not present.";
				m_runningStatus = ERunningStatus::ExceptionOccurred;
				return;
			}

			//
			// retrieve the query blueprint for a type check below
			// (it could be that the blueprint has been reloaded after it was changed by the designer [at runtime!] and is now producing a different kind of items)
			//

			const core::IQueryBlueprint* pQueryBlueprint = pHub->GetQueryBlueprintLibrary().GetQueryBlueprintByID(m_queryBlueprintID);
			if (!pQueryBlueprint)
			{
				m_exceptionMessageIfAny.Format("Query blueprint '%s' is not present in the blueprint library.", m_queryBlueprintID.GetQueryBlueprintName());
				m_runningStatus = ERunningStatus::ExceptionOccurred;
				return;
			}

			//
			// if the caller expects a certain item type in the result set then verify it
			//

			if (m_pExpectedOutputType)
			{
				const shared::CTypeInfo& typeOfGeneratedItems = pQueryBlueprint->GetOutputType();

				if (*m_pExpectedOutputType != typeOfGeneratedItems)
				{
					m_exceptionMessageIfAny.Format("Query blueprint '%s': item type mismatch: generated items are of type %s, but we expected %s", m_queryBlueprintID.GetQueryBlueprintName(), typeOfGeneratedItems.name(), m_pExpectedOutputType->name());
					m_runningStatus = ERunningStatus::ExceptionOccurred;
					return;
				}
			}

			//
			// start the query
			//

			const SQueryRequest queryRequest(m_queryBlueprintID, m_runtimeParams, m_querierName.c_str(), functor(*this, &CQueryHost::OnUQSQueryFinished));
			uqs::shared::CUqsString error;
			m_queryID = pHub->GetQueryManager().StartQuery(queryRequest, error);
			m_runningStatus = ERunningStatus::StillRunning;
		}

		bool CQueryHost::IsStillRunning() const
		{
			return m_runningStatus == ERunningStatus::StillRunning;
		}

		bool CQueryHost::HasSucceeded() const
		{
			return m_runningStatus == ERunningStatus::FinishedWithSuccess;
		}

		const core::IQueryResultSet& CQueryHost::GetResultSet() const
		{
			assert(m_runningStatus == ERunningStatus::FinishedWithSuccess);
			assert(m_pResultSet);
			return *m_pResultSet;
		}

		const char* CQueryHost::GetExceptionMessage() const
		{
			assert(m_runningStatus == ERunningStatus::ExceptionOccurred);
			return m_exceptionMessageIfAny.c_str();
		}

		void CQueryHost::ClearResultSet()
		{
			m_pResultSet.reset();
		}

		void CQueryHost::OnUQSQueryFinished(const core::SQueryResult& result)
		{
			assert(result.queryID == m_queryID);

			m_queryID = core::CQueryID::CreateInvalid();

			switch (result.status)
			{
			case core::SQueryResult::EStatus::Success:
				m_pResultSet = std::move(result.pResultSet);
				m_runningStatus = ERunningStatus::FinishedWithSuccess;
				break;

			case core::SQueryResult::EStatus::ExceptionOccurred:
				m_exceptionMessageIfAny = result.error;
				m_runningStatus = ERunningStatus::ExceptionOccurred;
				break;

			case core::SQueryResult::EStatus::CanceledByHubTearDown:
				// FIXME: is it OK to treat this as an exception as well? (after all, the query hasn't fully finished!)
				m_exceptionMessageIfAny = "Query got canceled due to Hub being torn down now.";
				m_runningStatus = ERunningStatus::ExceptionOccurred;
				break;

			default:
				assert(0);
				m_exceptionMessageIfAny = "CQueryHost::OnUQSQueryFinished: unhandled status enum.";
				m_runningStatus = ERunningStatus::ExceptionOccurred;
				break;
			}
		}

	}
}
