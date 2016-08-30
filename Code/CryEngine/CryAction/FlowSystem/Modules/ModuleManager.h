// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryFlowGraph/IFlowGraphModuleManager.h>
#include <CryCore/Containers/CryListenerSet.h>
#include <CryGame/IGameFramework.h>

struct SActivationInfo;
class CFlowGraphModule;
class CFlowModuleCallNode;

class CFlowGraphModuleManager : public IFlowGraphModuleManager, public ISystemEventListener, public IGameFrameworkListener
{
public:
	////////////////////////////////////////////////////
	CFlowGraphModuleManager();
	virtual ~CFlowGraphModuleManager();

	// ISystemEventListener
	virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
	// ~ISystemEventListener

	// IFlowGraphModuleManager
	virtual bool               RegisterListener(IFlowGraphModuleListener* pListener, const char* name);
	virtual void               UnregisterListener(IFlowGraphModuleListener* pListener);

	virtual bool               DeleteModuleXML(const char* moduleName);
	virtual bool               RenameModuleXML(const char* moduleName, const char* newName);

	virtual bool               SaveModule(const char* moduleName, XmlNodeRef saveTo);
	IFlowGraphModule*          LoadModuleFile(const char* moduleName, const char* fileName, bool bGlobal);

	virtual IModuleIteratorPtr CreateModuleIterator();

	virtual const char*        GetStartNodeName(const char* moduleName) const;
	virtual const char*        GetReturnNodeName(const char* moduleName) const;
	virtual const char*        GetCallerNodeName(const char* moduleName) const;
	virtual void               ScanForModules();
	virtual const char*        GetModulePath(const char* name);
	virtual bool               CreateModuleNodes(const char* moduleName, TModuleId moduleId);

	virtual IFlowGraphModule*  GetModule(IFlowGraphPtr pFlowgraph) const;
	virtual IFlowGraphModule*  GetModule(const char* moduleName) const;
	virtual IFlowGraphModule*  GetModule(TModuleId id) const;
	virtual TModuleInstanceId  CreateModuleInstance(TModuleId moduleId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback);
	virtual TModuleInstanceId  CreateModuleInstance(TModuleId moduleId, TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback);
	// ~IFlowGraphModuleManager

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame)         {}
	virtual void OnLoadGame(ILoadGame* pLoadGame)         {}
	virtual void OnLevelEnd(const char* nextLevel)        {}
	virtual void OnActionEvent(const SActionEvent& event) {}
	// ~IGameFrameworkListener

	void Shutdown();

	void ClearModules();                   // Unload all loaded modules
	void RemoveCompletedModuleInstances(); // cleanup at end of update

	bool AddModulePathInfo(const char* moduleName, const char* path);

	void RefreshModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId, TModuleParams const& params);
	void CancelModuleInstance(TModuleId moduleId, TModuleInstanceId instanceId);

	// OnModuleFinished: Called when a module is done executing
	void OnModuleFinished(TModuleId const& moduleId, TModuleInstanceId instanceId, bool bSuccess, TModuleParams const& params);

	DeclareStaticConstIntCVar(CV_fg_debugmodules, 0);
	ICVar* fg_debugmodules_filter;

private:
	CFlowGraphModuleManager(CFlowGraphModuleManager const&) : m_listeners(1) {}
	// cppcheck-suppress operatorEqVarError
	CFlowGraphModuleManager& operator=(CFlowGraphModuleManager const&) { return *this; }
	void                     RescanModuleNames(bool bGlobal);
	void                     ScanFolder(const string& folderName, bool bGlobal);

	CFlowGraphModule*        PreLoadModuleFile(const char* moduleName, const char* fileName, bool bGlobal);
	void                     LoadModuleGraph(const char* moduleName, const char* fileName, IFlowGraphModuleListener::ERootGraphChangeReason rootGraphChangeReason);

	void                     DestroyActiveModuleInstances();

	// Module Id map
	typedef std::map<string, TModuleId> TModuleIdMap;
	TModuleIdMap m_ModuleIds;

	// Loaded modules
	typedef std::map<TModuleId, CFlowGraphModule*> TModuleMap;
	TModuleMap m_Modules;
	TModuleId  m_moduleIdMaker;

	typedef std::map<string, string> TModulesPathInfo;
	TModulesPathInfo                        m_ModulesPathInfo;

	CListenerSet<IFlowGraphModuleListener*> m_listeners;

	class CModuleIterator : public IFlowGraphModuleIterator
	{
	public:
		CModuleIterator(CFlowGraphModuleManager* pMM)
		{
			m_pModuleManager = pMM;
			m_cur = m_pModuleManager->m_Modules.begin();
			m_nRefs = 0;
		}
		void AddRef()
		{
			++m_nRefs;
		}
		void Release()
		{
			if (--m_nRefs <= 0)
			{
				this->~CModuleIterator();
				m_pModuleManager->m_iteratorPool.push_back(this);
			}
		}
		IFlowGraphModule* Next();
		size_t            Count()
		{
			return m_pModuleManager->m_Modules.size();
		}
		CFlowGraphModuleManager* m_pModuleManager;
		TModuleMap::iterator     m_cur;
		int                      m_nRefs;
	};

	std::vector<CModuleIterator*> m_iteratorPool;
};
