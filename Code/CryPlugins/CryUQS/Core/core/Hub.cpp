// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Hub.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// Hub_HaveConsistencyChecksBeenDoneAlready()
		//
		//===================================================================================

		bool Hub_HaveConsistencyChecksBeenDoneAlready()
		{
			assert(g_hubImpl);
			return g_hubImpl->HaveConsistencyChecksBeenDoneAlready();
		}

		//===================================================================================
		//
		// CHub
		//
		//===================================================================================

		CHub* g_hubImpl;

		CHub::CHub()
			: m_consistencyChecksDoneAlready(false)
			, m_queryHistoryInGameGUI(m_queryHistoryManager)
			, m_queryManager(m_queryHistoryManager)
			, m_pEditorLibraryProvider(nullptr)
		{
			assert(!g_hubImpl);
			g_hubImpl = this;
			GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
			m_utils.SubscribeToStuffInHub(*this);

			CQueryFactoryBase::RegisterAllInstancesInDatabase(m_queryFactoryDatabase);

			SCvars::Register();
			REGISTER_COMMAND("UQS_ListFactoryDatabases", CmdListFactoryDatabases, 0, "Prints all registered factories for creating items, functions, generators and evaluators to the console.");
			REGISTER_COMMAND("UQS_ListQueryBlueprintLibrary", CmdListQueryBlueprintLibrary, 0, "Prints all query-blueprints in the library to the console.");
			REGISTER_COMMAND("UQS_ListRunningQueries", CmdListRunningQueries, 0, "Prints all currently running queries to the console.");
			REGISTER_COMMAND("UQS_DumpQueryHistory", CmdDumpQueryHistory, 0, "Dumps all queries that were executed so far to an XML file for de-serialization at a later time.");
			REGISTER_COMMAND("UQS_LoadQueryHistory", CmdLoadQueryHistory, 0, "Loads a history of queries from an XML for debug-rendering and inspection in the 3D world.");
			REGISTER_COMMAND("UQS_ClearLiveQueryHistory", CmdClearLiveQueryHistory, 0, "Clears the history of currently ongoing queries in memory.");
			REGISTER_COMMAND("UQS_ClearDeserialzedQueryHistory", CmdClearDeserializedQueryHistory, 0, "Clears the history of queries previously loaded from disk into memory.");
		}

		CHub::~CHub()
		{
			g_hubImpl = nullptr;
			GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
			m_utils.UnsubscribeFromStuffInHub(*this);
			SCvars::Unregister();
		}

		void CHub::RegisterHubEventListener(IHubEventListener* listener)
		{
			stl::push_back_unique(m_eventListeners, listener);
		}

		void CHub::Update()
		{
			//
			// query manager
			//

			m_queryManager.Update();

			//
			// history drawing (only 2D texts; the 3D debug primitives of each historic query are done via IQueryHistoryManager::UpdateDebugRendering3D())
			//

			if (SCvars::debugDraw)
			{
				//
				// ongoing queries as 2D text
				//

				m_queryManager.DebugDrawRunningQueriesStatistics2D();

				//
				// draw the list of historic queries along with detailed information about the one that is currently focused by the camera
				//

				m_queryHistoryInGameGUI.Draw();
			}
		}

		QueryFactoryDatabase& CHub::GetQueryFactoryDatabase()
		{
			return m_queryFactoryDatabase;
		}

		ItemFactoryDatabase& CHub::GetItemFactoryDatabase()
		{
			return m_itemFactoryDatabase;
		}

		FunctionFactoryDatabase& CHub::GetFunctionFactoryDatabase()
		{
			return m_functionFactoryDatabase;
		}

		GeneratorFactoryDatabase& CHub::GetGeneratorFactoryDatabase()
		{
			return m_generatorFactoryDatabase;
		}

		InstantEvaluatorFactoryDatabase& CHub::GetInstantEvaluatorFactoryDatabase()
		{
			return m_instantEvaluatorFactoryDatabase;
		}

		DeferredEvaluatorFactoryDatabase& CHub::GetDeferredEvaluatorFactoryDatabase()
		{
			return m_deferredEvaluatorFactoryDatabase;
		}

		CQueryBlueprintLibrary& CHub::GetQueryBlueprintLibrary()
		{
			return m_queryBlueprintLibrary;
		}

		CQueryManager& CHub::GetQueryManager()
		{
			return m_queryManager;
		}

		CQueryHistoryManager& CHub::GetQueryHistoryManager()
		{
			return m_queryHistoryManager;
		}

		CUtils& CHub::GetUtils()
		{
			return m_utils;
		}

		CEditorService& CHub::GetEditorService()
		{
			return m_editorService;
		}

		CItemSerializationSupport& CHub::GetItemSerializationSupport()
		{
			return m_itemSerializationSupport;
		}

		datasource::IEditorLibraryProvider* CHub::GetEditorLibraryProvider()
		{
			return m_pEditorLibraryProvider;
		}

		void CHub::SetEditorLibraryProvider(datasource::IEditorLibraryProvider* pProvider)
		{
			m_pEditorLibraryProvider = pProvider;
		}

		bool CHub::HaveConsistencyChecksBeenDoneAlready() const
		{
			return m_consistencyChecksDoneAlready;
		}

		void CHub::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
		{
			if (event == ESYSTEM_EVENT_GAME_POST_INIT_DONE)
			{
				//
				// - ask all sub-systems to register their factories (before doing the consistency checks below!!!)
				// - the sub-systems should have used ESYSTEM_EVENT_GAME_POST_INIT (*not* the _DONE event) to subscribe to the IHub for receiving events
				//

				SendHubEventToAllListeners(uqs::core::EHubEvent::RegisterYourFactoriesNow);

				//
				// check for consistency errors (this needs to be done *after* all subsystems registered their item types, functions, generators, evaluators)
				//

				CStartupConsistencyChecker startupConsistencyChecker;
				startupConsistencyChecker.CheckForConsistencyErrors();
				if (const size_t numErrors = startupConsistencyChecker.GetErrorCount())
				{
					for (size_t i = 0; i < numErrors; ++i)
					{
						if (gEnv->IsEditor())
						{
							// prepend "!" to the error message to make the error report pop up in Sandbox
							gEnv->pSystem->Warning(VALIDATOR_MODULE_UNKNOWN, VALIDATOR_ERROR, 0, 0, "!UQS consistency check: %s", startupConsistencyChecker.GetError(i));
						}
						else
						{
							gEnv->pLog->LogError("UQS consistency check: %s", startupConsistencyChecker.GetError(i));
						}
					}
				}

				// from now on, don't allow any further factory registrations (uqs::core::CFactoryDatabase<>::RegisterFactory() will assert for it)
				m_consistencyChecksDoneAlready = true;

				//
				// tell the game (or whoever "owns" the UQS instance) to load the query blueprints
				//

				SendHubEventToAllListeners(EHubEvent::LoadQueryBlueprintLibrary);
			}
		}

		void CHub::SendHubEventToAllListeners(EHubEvent ev)
		{
			for (std::list<IHubEventListener*>::const_iterator it = m_eventListeners.cbegin(); it != m_eventListeners.cend(); )
			{
				IHubEventListener* listener = *it++;
				listener->OnUQSHubEvent(ev);
			}
		}

		void CHub::CmdListFactoryDatabases(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				CLogger logger;
				g_hubImpl->m_queryFactoryDatabase.PrintToConsole(logger, "Query");
				g_hubImpl->m_itemFactoryDatabase.PrintToConsole(logger, "Item");
				g_hubImpl->m_functionFactoryDatabase.PrintToConsole(logger, "Function");
				g_hubImpl->m_generatorFactoryDatabase.PrintToConsole(logger, "Generator");
				g_hubImpl->m_instantEvaluatorFactoryDatabase.PrintToConsole(logger, "InstantEvaluator");
				g_hubImpl->m_deferredEvaluatorFactoryDatabase.PrintToConsole(logger, "DeferredEvaluator");
			}
		}

		void CHub::CmdListQueryBlueprintLibrary(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				CLogger logger;
				g_hubImpl->m_queryBlueprintLibrary.PrintToConsole(logger);
			}
		}

		void CHub::CmdListRunningQueries(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				CLogger logger;
				g_hubImpl->m_queryManager.PrintRunningQueriesToConsole(logger);
			}
		}

		void CHub::CmdDumpQueryHistory(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				//
				// create an XML filename with a unique counter as part of it
				//

				stack_string unadjustedFilePath;

				for (int uniqueCounter = 0; uniqueCounter < 9999; ++uniqueCounter)
				{
					unadjustedFilePath.Format("%%USER%%/UQS_Logs/QueryHistory_%04i.xml", uniqueCounter);
					if (!gEnv->pCryPak->IsFileExist(unadjustedFilePath))	// no need to call gEnv->pCryPak->AdjustFileName() beforehand
						break;
				}

				char adjustedFilePath[ICryPak::g_nMaxPath] = "";

				if (!gEnv->pCryPak->AdjustFileName(unadjustedFilePath.c_str(), adjustedFilePath, ICryPak::FLAGS_FOR_WRITING))
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: Could not adjust the desired file path '%s' for writing", pArgs->GetArg(0), unadjustedFilePath.c_str());
					return;
				}

				shared::CUqsString error;
				if (!g_hubImpl->m_queryHistoryManager.SerializeLiveQueryHistory(adjustedFilePath, error))
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: Serializing the live query to '%s' failed: %s", pArgs->GetArg(0), unadjustedFilePath.c_str(), error.c_str());
					return;
				}

				CryLogAlways("Successfully dumped query history to '%s'", adjustedFilePath);
			}
		}

		void CHub::CmdLoadQueryHistory(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				if (pArgs->GetArgCount() < 2)
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "usage: %s <xml query history file>", pArgs->GetArg(0));
					return;
				}

				const char* xmlQueryHistoryFilePath = pArgs->GetArg(1);

				// check if the desired XML file exists at all (just for giving a more precise warning)
				if (!gEnv->pCryPak->IsFileExist(xmlQueryHistoryFilePath))	// no need to call gEnv->pCryPak->AdjustFileName() beforehand
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: File not found: '%s'", pArgs->GetArg(0), xmlQueryHistoryFilePath);
					return;
				}

				shared::CUqsString error;
				if (!g_hubImpl->m_queryHistoryManager.DeserializeQueryHistory(xmlQueryHistoryFilePath, error))
				{
					CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: Could not de-serialize the query history: ", pArgs->GetArg(0), xmlQueryHistoryFilePath, error.c_str());
					return;
				}

				CryLogAlways("Successfully de-serialized '%s'", xmlQueryHistoryFilePath);
			}
		}

		void CHub::CmdClearLiveQueryHistory(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				g_hubImpl->m_queryHistoryManager.ClearQueryHistory(IQueryHistoryManager::EHistoryOrigin::Live);
			}
		}

		void CHub::CmdClearDeserializedQueryHistory(IConsoleCmdArgs* pArgs)
		{
			if (g_hubImpl)
			{
				g_hubImpl->m_queryHistoryManager.ClearQueryHistory(IQueryHistoryManager::EHistoryOrigin::Deserialized);
			}
		}

	}
}
