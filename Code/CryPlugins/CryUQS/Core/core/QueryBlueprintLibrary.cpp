// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QueryBlueprintLibrary.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		CQueryBlueprintLibrary::CQueryBlueprintLibrary()
		{
			// nothing
		}

		CQueryBlueprintLibrary::ELoadAndStoreResult CQueryBlueprintLibrary::LoadAndStoreQueryBlueprint(ELoadAndStoreOverwriteBehavior overwriteBehavior, datasource::IQueryBlueprintLoader& loader, shared::IUqsString& error)
		{
			CTextualQueryBlueprint textualQueryBP;

			if (!loader.LoadTextualQueryBlueprint(textualQueryBP, error))
			{
				return ELoadAndStoreResult::ExceptionOccurred;
			}

			const char* queryBPName = textualQueryBP.GetName();
			const bool queryBPExistsAlready = (FindQueryBlueprintByName(queryBPName) != nullptr);

			if (queryBPExistsAlready && overwriteBehavior == ELoadAndStoreOverwriteBehavior::KeepExisting)
			{
				return ELoadAndStoreResult::PreservedExistingOne;
			}

			std::shared_ptr<CQueryBlueprint> newBP(new CQueryBlueprint);
			if (!newBP->Resolve(textualQueryBP))
			{
				error.Format("Could not resolve the textual query-blueprint '%s'", queryBPName);
				return ELoadAndStoreResult::ExceptionOccurred;
			}

			assert(strcmp(queryBPName, newBP->GetName()) == 0);

			m_library[queryBPName] = newBP;

			return queryBPExistsAlready ? ELoadAndStoreResult::OverwrittenExistingOne : ELoadAndStoreResult::StoredFromScratch;
		}

		bool CQueryBlueprintLibrary::RemoveStoredQueryBlueprint(const char* szQueryBlueprintName, shared::IUqsString& error)
		{
			auto it = m_library.find(szQueryBlueprintName);
			if (it == m_library.cend())
			{
				error.Format("Could not remove query-blueprint '%s': not found", szQueryBlueprintName);
				return false;
			}

			m_library.erase(it);
			return true;
		}

		std::shared_ptr<const CQueryBlueprint> CQueryBlueprintLibrary::FindQueryBlueprintByName(const char* queryBlueprintName) const
		{
			auto it = m_library.find(queryBlueprintName);
			return (it == m_library.cend()) ? nullptr : it->second;
		}

		void CQueryBlueprintLibrary::PrintToConsole(CLogger& logger) const
		{
			logger.Printf("");
			logger.Printf("=== %i UQS query blueprints in the library ===", (int)m_library.size());
			logger.Printf("");
			CLoggerIndentation _indent;

			for (const auto& entry : m_library)
			{
				const CQueryBlueprint* queryBlueprint = entry.second.get();
				queryBlueprint->PrintToConsole(logger);
				logger.Printf("");
			}
		}

	}
}
