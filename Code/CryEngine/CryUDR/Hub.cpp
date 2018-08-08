// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry 
{
	namespace UDR
	{

		CHub* CHub::s_pInstance;

		CHub::CHub()
			: m_treeManager()
			, m_nodeStack(m_treeManager.GetTree(ITreeManager::ETreeIndex::Live).GetRootNodeWritable())
		{
			CRY_ASSERT(!s_pInstance);
			s_pInstance = this;

			SCvars::Register();

			REGISTER_COMMAND("UDR_DumpRecordings", CmdDumpRecordings, 0, "Dumps all recordings to an XML file for de-serialization at a later time.");
			REGISTER_COMMAND("UDR_ClearRecordingsInLiveTree", CmdClearRecordingsInLiveTree, 0, "Clears all recordings that are currently in memory in the live tree.");
			REGISTER_COMMAND("UDR_ClearRecordingsInDeserializedTree", CmdClearRecordingsInDeserializedTree, 0, "Clears all recordings that are currently in memory in the deserialzed tree.");
			REGISTER_COMMAND("UDR_PrintLiveNodes", CmdPrintLiveNodes, 0, "");
			REGISTER_COMMAND("UDR_PrintStatistics", CmdPrintStatistics, 0, "");
		}

		CHub::~CHub()
		{
			SCvars::Unregister();

			CRY_ASSERT(s_pInstance == this);
			s_pInstance = nullptr;
		}

		ITreeManager& CHub::GetTreeManager()
		{
			return m_treeManager;
		}

		INodeStack& CHub::GetNodeStack()
		{
			return m_nodeStack;
		}

		IRecursiveSyncObject& CHub::GetRecursiveSyncObject()
		{
			return m_recursiveSyncObject;
		}

		void CHub::CmdDumpRecordings(IConsoleCmdArgs* pArgs)
		{
			if (s_pInstance)
			{
				{
					CRecursiveSyncObjectAutoLock _lock;

					//
					// create an XML filename using a unique counter as suffix
					//

					stack_string unadjustedFilePath;
					bool foundUnusedFile = false;

					for (int uniqueCounter = 0; uniqueCounter < 9999; ++uniqueCounter)
					{
						unadjustedFilePath.Format("%%USER%%/UDR_Recordings/Recording_%04i.xml", uniqueCounter);
						if (!gEnv->pCryPak->IsFileExist(unadjustedFilePath))  // no need to call gEnv->pCryPak->AdjustFileName() beforehand
						{
							foundUnusedFile = true;
							break;
						}
					}

					if (!foundUnusedFile)
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: No more unused files write to found (refusing to overwrite '%s')", pArgs->GetArg(0), unadjustedFilePath.c_str());
						return;
					}

					char adjustedFilePath[ICryPak::g_nMaxPath] = "";

					if (!gEnv->pCryPak->AdjustFileName(unadjustedFilePath.c_str(), adjustedFilePath, ICryPak::FLAGS_FOR_WRITING))
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: Could not adjust the desired file path '%s' for writing", pArgs->GetArg(0), unadjustedFilePath.c_str());
						return;
					}

					CString error;
					if (!s_pInstance->m_treeManager.SerializeLiveTree(adjustedFilePath, error))
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "%s: Serializing the recordings to '%s' failed: %s", pArgs->GetArg(0), unadjustedFilePath.c_str(), error.c_str());
						return;
					}

					CryLogAlways("Successfully dumped all recordings to '%s'", adjustedFilePath);
				}
			}
		}

		void CHub::CmdClearRecordingsInLiveTree(IConsoleCmdArgs* pArgs)
		{
			HelpCmdClearRecordings(pArgs, ITreeManager::ETreeIndex::Live);
		}

		void CHub::CmdClearRecordingsInDeserializedTree(IConsoleCmdArgs* pArgs)
		{
			HelpCmdClearRecordings(pArgs, ITreeManager::ETreeIndex::Deserialized);
		}

		void CHub::CmdPrintLiveNodes(IConsoleCmdArgs* pArgs)
		{
			if (s_pInstance)
			{
				{
					CRecursiveSyncObjectAutoLock _lock;

					CryLogAlways("=== UDR nodes in the live tree ===");
					s_pInstance->m_treeManager.GetTree(ITreeManager::ETreeIndex::Live).GetRootNode().PrintHierarchyToConsoleRecursively(0);
				}
			}
		}

		void CHub::CmdPrintStatistics(IConsoleCmdArgs* pArgs)
		{
			if (s_pInstance)
			{
				{
					CRecursiveSyncObjectAutoLock _lock;

					CryLogAlways("=== UDR statistics ===");
					s_pInstance->m_treeManager.GetTree(ITreeManager::ETreeIndex::Live).GetRootNode().PrintStatisticsToConsole("Live tree:         ");
					s_pInstance->m_treeManager.GetTree(ITreeManager::ETreeIndex::Deserialized).GetRootNode().PrintStatisticsToConsole("Deserialized tree: ");
				}
			}
		}

		void CHub::HelpCmdClearRecordings(IConsoleCmdArgs* pArgs, ITreeManager::ETreeIndex treeIndex)
		{
			if (s_pInstance)
			{
				{
					CRecursiveSyncObjectAutoLock _lock;

					CNode& root = s_pInstance->m_treeManager.GetTree(treeIndex).GetRootNodeWritable();
					CNode::SStatistics stats;
					root.GatherStatisticsRecursively(stats);
					root.RemoveChildren();
					CryLogAlways("Removed %i nodes and %i render prims from the %s tree.", (int)stats.numChildren, (int)stats.numRenderPrimitives, treeIndex == ITreeManager::ETreeIndex::Live ? "live" : "deserialized");
				}
			}
		}

	}
}
