// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/********************************************************************
   -------------------------------------------------------------------------
   File name:   IFlowGraphModuleManager.h
   $Id$
   Description: IFlowGraphModuleManager interface

   -------------------------------------------------------------------------
   History:
   - 03/04/11   : Sascha Hoba - Kevin Kirst

 *********************************************************************/

#ifndef FLOWGRAPHMODULEMANAGERINTERFACE_H
#define FLOWGRAPHMODULEMANAGERINTERFACE_H

#if _MSC_VER > 1000
	#pragma once
#endif // _MSC_VER > 1000

#include <CryFlowGraph/IFlowSystem.h>

//! Module Ids.
typedef uint TModuleId;
static const TModuleId MODULEID_INVALID = -1;

//! Instance Ids.
typedef uint TModuleInstanceId;
static const TModuleInstanceId MODULEINSTANCE_INVALID = -1;

//! Parameter passing object.
typedef DynArray<TFlowInputData>             TModuleParams;
typedef Functor2<bool, const TModuleParams&> ModuleInstanceReturnCallback;

struct IModuleInstance
{
	IFlowGraphPtr                pGraph;
	TFlowGraphId                 callerGraph;
	TFlowNodeId                  callerNode;
	ModuleInstanceReturnCallback returnCallback;
	TModuleInstanceId            instanceId;
	bool                         bUsed;

	IModuleInstance(TModuleInstanceId id) : pGraph(NULL), callerGraph(InvalidFlowGraphId), callerNode(InvalidFlowNodeId), bUsed(false), instanceId(id), returnCallback(NULL) {}
};

struct IFlowGraphModuleInstanceIterator
{
	virtual ~IFlowGraphModuleInstanceIterator(){}
	virtual size_t           Count() = 0;
	virtual IModuleInstance* Next() = 0;
	virtual void             AddRef() = 0;
	virtual void             Release() = 0;
};
typedef _smart_ptr<IFlowGraphModuleInstanceIterator> IModuleInstanceIteratorPtr;

struct IFlowGraphModule
{
	enum EType
	{
		eT_Global = 0,
		eT_Level
	};

	virtual ~IFlowGraphModule() {}

	struct SModulePortConfig
	{
		SModulePortConfig() : type(eFDT_Void), input(true) {}

		inline bool operator==(const SModulePortConfig& other)
		{
			return (name == other.name) && (type == other.type) && (input == other.input);
		}

		string         name;
		EFlowDataTypes type;
		bool           input;
	};

	virtual const char*                                GetName() const = 0;
	virtual const char*                                GetPath() const = 0;
	virtual TModuleId                                  GetId() const = 0;
	virtual IFlowGraphModule::EType                    GetType() const = 0;
	virtual void                                       SetType(IFlowGraphModule::EType type) = 0;

	virtual IFlowGraph*                                GetRootGraph() const = 0;
	virtual IFlowGraphPtr                              GetInstanceGraph(TModuleInstanceId instanceID) = 0;

	virtual IModuleInstanceIteratorPtr                 CreateInstanceIterator() = 0;

	virtual void                                       RemoveModulePorts() = 0;
	virtual bool                                       AddModulePort(const SModulePortConfig& port) = 0;
	virtual size_t                                     GetModulePortCount() const = 0;
	virtual const IFlowGraphModule::SModulePortConfig* GetModulePort(size_t index) const = 0;
};

typedef _smart_ptr<IFlowGraphModule> IFlowGraphModulePtr;

struct IFlowGraphModuleIterator
{
	virtual ~IFlowGraphModuleIterator(){}
	virtual size_t            Count() = 0;
	virtual IFlowGraphModule* Next() = 0;
	virtual void              AddRef() = 0;
	virtual void              Release() = 0;
};
typedef _smart_ptr<IFlowGraphModuleIterator> IModuleIteratorPtr;

struct IFlowGraphModuleListener
{
	enum class ERootGraphChangeReason
	{
		ScanningForModules,
		LoadModuleFile
	};

	//! Called once a new module instance was created.
	virtual void OnModuleInstanceCreated(IFlowGraphModule* module, TModuleInstanceId instanceID) = 0;

	//! Called once a new module instance was destroyed.
	virtual void OnModuleInstanceDestroyed(IFlowGraphModule* module, TModuleInstanceId instanceID) = 0;

	//! Called once a module was destroyed.
	virtual void OnModuleDestroyed(IFlowGraphModule* module) = 0;

	//! Called right after a module was destroyed.
	virtual void OnPostModuleDestroyed() = 0;

	//! Called once a modules root graph has changed.
	virtual void OnRootGraphChanged(IFlowGraphModule* module, ERootGraphChangeReason reason) = 0;

	//! Called once the system scanned for new modules.
	virtual void OnScannedForModules() = 0;

protected:
	virtual ~IFlowGraphModuleListener() {};
};

struct IFlowGraphModuleManager
{
	virtual ~IFlowGraphModuleManager() {}

	virtual bool               RegisterListener(IFlowGraphModuleListener* pListener, const char* name) = 0;
	virtual void               UnregisterListener(IFlowGraphModuleListener* pListener) = 0;

	virtual bool               DeleteModuleXML(const char* moduleName) = 0;
	virtual bool               RenameModuleXML(const char* moduleName, const char* newName) = 0;
	virtual IModuleIteratorPtr CreateModuleIterator() = 0;

	virtual const char*        GetStartNodeName(const char* moduleName) const = 0;
	virtual const char*        GetReturnNodeName(const char* moduleName) const = 0;
	virtual const char*        GetCallerNodeName(const char* moduleName) const = 0;
	virtual void               ScanForModules() = 0;
	virtual const char*        GetModulePath(const char* name) = 0;

	virtual bool               SaveModule(const char* moduleName, XmlNodeRef saveTo) = 0;
	virtual IFlowGraphModule*  LoadModuleFile(const char* moduleName, const char* fileName, bool bGlobal) = 0;

	virtual IFlowGraphModule*  GetModule(IFlowGraphPtr pFlowgraph) const = 0;
	virtual IFlowGraphModule*  GetModule(const char* moduleName) const = 0;
	virtual IFlowGraphModule*  GetModule(TModuleId id) const = 0;

	virtual TModuleInstanceId  CreateModuleInstance(TModuleId moduleId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback) = 0;
	virtual TModuleInstanceId  CreateModuleInstance(TModuleId moduleId, TFlowGraphId callerGraphId, TFlowNodeId callerNodeId, const TModuleParams& params, const ModuleInstanceReturnCallback& returnCallback) = 0;
	virtual bool               CreateModuleNodes(const char* moduleName, TModuleId moduleId) = 0;
};

#endif //FLOWGRAPHMODULEMANAGERINTERFACE_H
