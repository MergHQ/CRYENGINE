// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		class CHub final : public IHub
		{
		public:

			explicit						CHub();
			~CHub();

			// IHub
			virtual ITreeManager&			GetTreeManager() override;
			virtual INodeStack&				GetNodeStack() override;
			virtual IRecursiveSyncObject&	GetRecursiveSyncObject() override;
			// ~IHub

		private:

			static void						CmdDumpRecordings(IConsoleCmdArgs* pArgs);
			static void						CmdClearRecordingsInLiveTree(IConsoleCmdArgs* pArgs);
			static void						CmdClearRecordingsInDeserializedTree(IConsoleCmdArgs* pArgs);
			static void						CmdPrintLiveNodes(IConsoleCmdArgs* pArgs);
			static void						CmdPrintStatistics(IConsoleCmdArgs* pArgs);

			static void						HelpCmdClearRecordings(IConsoleCmdArgs* pArgs, ITreeManager::ETreeIndex treeIndex);

		private:

			CTreeManager					m_treeManager;
			CNodeStack						m_nodeStack;
			CRecursiveSyncObject			m_recursiveSyncObject;

			static CHub*					s_pInstance;
		};

	}
}

