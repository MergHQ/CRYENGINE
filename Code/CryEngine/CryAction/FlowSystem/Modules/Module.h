// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/********************************************************************
   -------------------------------------------------------------------------
   File name:   Module.h
   $Id$
   Description: Module container

   -------------------------------------------------------------------------
   History:
   - 03/04/11   : Sascha Hoba - Kevin Kirst

 *********************************************************************/

#ifndef _MODULE_H_
#define _MODULE_H_

#include <CryFlowGraph/IFlowGraphModuleManager.h>

class CFlowModuleCallNode;

class CFlowGraphModule : public IFlowGraphModule
{
	friend class CFlowGraphModuleManager;

public:

	CFlowGraphModule(TModuleId moduleId);
	virtual ~CFlowGraphModule();

	// IFlowGraphModule
	virtual const char*                                GetName() const                       { return m_name.c_str(); }
	virtual const char*                                GetPath() const                       { return m_fileName.c_str(); }
	virtual TModuleId                                  GetId() const                         { return m_Id; }
	virtual IFlowGraphModule::EType                    GetType() const                       { return m_type; }
	virtual void                                       SetType(IFlowGraphModule::EType type) { m_type = type; }

	virtual IFlowGraph*                                GetRootGraph() const                  { return m_pRootGraph; }
	virtual IFlowGraphPtr                              GetInstanceGraph(TModuleInstanceId instanceID);

	virtual IModuleInstanceIteratorPtr                 CreateInstanceIterator();

	virtual void                                       RemoveModulePorts();
	virtual bool                                       AddModulePort(const SModulePortConfig& port);
	virtual size_t                                     GetModulePortCount() const;
	virtual const IFlowGraphModule::SModulePortConfig* GetModulePort(size_t index) const;
	// ~IFlowGraphModule

	// returns true if the specified graph belongs to an instance of this module
	bool HasInstanceGraph(IFlowGraphPtr pGraph);

private:

	bool PreLoadModule(const char* fileName);                           // load initial data, create nodes
	bool LoadModuleGraph(const char* moduleName, const char* fileName); // load actual flowgraph
	bool SaveModuleXml(XmlNodeRef saveTo);

	void Destroy();

	// Instance def
	struct SInstance : public IModuleInstance
	{
		SInstance(TModuleInstanceId id) : IModuleInstance(id) {}

		bool operator==(const SInstance& other)
		{
			return (pGraph == other.pGraph && callerGraph == other.callerGraph && callerNode == other.callerNode
			        && instanceId == other.instanceId && bUsed == other.bUsed);
		}
	};

	void ActivateGraph(IFlowGraph* pGraph, TModuleInstanceId instanceId, TModuleParams const& params) const;
	void DeactivateGraph(SInstance& instance, bool bSuccess, TModuleParams const& params) const;

	// create flowgraph nodes (Start, Return, Call) for this module
	void RegisterNodes();
	void UnregisterNodes();

	// Instance handling
	TModuleInstanceId CreateInstance(TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, TModuleParams const& params, const ModuleInstanceReturnCallback& returnCallback);
	void              RefreshInstance(TModuleInstanceId instanceId, TModuleParams const& params);
	void              CancelInstance(TModuleInstanceId instanceId);
	bool DestroyInstance(TModuleInstanceId, bool bSuccess, TModuleParams const & params);
	void              RemoveCompletedInstances();
	void              RemoveAllInstances();

private:
	TModuleId               m_Id;
	string                  m_name;
	string                  m_fileName;
	IFlowGraphModule::EType m_type;
	// Root graph all others are cloned from
	IFlowGraphPtr           m_pRootGraph;

	// Instance graphs
	typedef std::list<SInstance> TInstanceList;
	TInstanceList     m_instances;
	TModuleInstanceId m_nextInstanceId;

	// Inputs and outputs for this module
	typedef std::vector<SModulePortConfig> TModulePorts;
	TModulePorts m_modulePorts;

	// flow node registration
	TFlowNodeTypeId m_startNodeTypeId;
	TFlowNodeTypeId m_returnNodeTypeId;
	TFlowNodeTypeId m_callNodeTypeId;

	class CInstanceIterator : public IFlowGraphModuleInstanceIterator
	{
	public:
		CInstanceIterator(CFlowGraphModule* pFM)
		{
			m_pModule = pFM;
			m_cur = m_pModule->m_instances.begin();
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
				this->~CInstanceIterator();
				m_pModule->m_iteratorPool.push_back(this);
			}
		}
		IModuleInstance* Next();
		size_t           Count()
		{
			return m_pModule->m_instances.size();
		}
		CFlowGraphModule*       m_pModule;
		TInstanceList::iterator m_cur;
		int                     m_nRefs;
	};

	std::vector<CInstanceIterator*> m_iteratorPool;
};

TYPEDEF_AUTOPTR(CFlowGraphModule);
typedef CFlowGraphModule_AutoPtr CFlowGraphModulePtr;

#endif //_MODULE_H_
