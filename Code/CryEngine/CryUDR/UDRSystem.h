// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{
		class CUDRSystem final : public IUDRSystem
		{
		public:
			explicit CUDRSystem();
			~CUDRSystem();

			// IUDR
			virtual bool                  Initialize() override;
			virtual void                  Reset() override;
			virtual void                  Update(const CTimeValue frameStartTime, const float frameDeltaTime) override;
			
			virtual ITreeManager&         GetTreeManager() override;
			virtual INodeStack&           GetNodeStack() override;
			virtual IRecursiveSyncObject& GetRecursiveSyncObject() override;
			
			virtual bool                  SetConfig(const SConfig& config) override;
			virtual const SConfig&        GetConfig() const override;

			virtual CTimeValue            GetElapsedTime() const override;
			// ~IUDR

		private:
		
			static void                   CmdDumpRecordings(IConsoleCmdArgs* pArgs);
			static void                   CmdClearRecordingsInLiveTree(IConsoleCmdArgs* pArgs);
			static void                   CmdClearRecordingsInDeserializedTree(IConsoleCmdArgs* pArgs);
			static void                   CmdPrintLiveNodes(IConsoleCmdArgs* pArgs);
			static void                   CmdPrintStatistics(IConsoleCmdArgs* pArgs);

			static void                   HelpCmdClearRecordings(IConsoleCmdArgs* pArgs, ITreeManager::ETreeIndex treeIndex);

		private:

			static CUDRSystem*            s_pInstance;
			
			IUDRSystem::SConfig           m_config;
			CTreeManager                  m_treeManager;
			CNodeStack                    m_nodeStack;
			CRecursiveSyncObject          m_recursiveSyncObject;
			
			CTimeMetadata                 m_timeMetadataBase;
			CTimeValue                    m_frameStartTime;
			float                         m_frameDeltaTime = 0;
			
		};
	}
}