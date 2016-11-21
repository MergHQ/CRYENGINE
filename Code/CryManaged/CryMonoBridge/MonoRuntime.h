// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <mono/jit/jit.h>
#include <CrySystem/IConsole.h>
#include <CryMono/IMonoRuntime.h>
#include <CryCore/Containers/CryListenerSet.h>
#include <CryAISystem/BehaviorTree/IBehaviorTree.h>
#include <CryAISystem/BehaviorTree/Node.h>

#include "Wrappers/MonoObject.h"

class CMonoDomain;
class CRootMonoDomain;
class CAppDomain;

class CMonoLibrary;

class CManagedPlugin;

typedef CListenerSet<IMonoListener*>            MonoListeners;

enum EMonoLogLevel
{
	eMLL_NULL = 0,
	eMLL_Error,
	eMLL_Critical,
	eMLL_Warning,
	eMLL_Message,
	eMLL_Info,
	eMLL_Debug
};

class CManagedNodeCreatorProxy : public BehaviorTree::INodeCreator
{
public:
	CManagedNodeCreatorProxy(const char* szTypeName, IManagedNodeCreator* pCreator)
		: m_pCreator(pCreator)
		, m_pNodeFactory(nullptr) 
	{ 
		cry_strcpy(m_szTypeName, szTypeName);
	}

	// INodeCreator
	virtual ~CManagedNodeCreatorProxy()
	{
		delete m_pCreator;
	}
	virtual BehaviorTree::INodePtr    Create() override
	{
		return BehaviorTree::INodePtr(m_pCreator->Create());
	}
	virtual void        Trim() override {}
	virtual const char* GetTypeName() const override { return m_szTypeName; }
	virtual size_t      GetNodeClassSize() const override { return 4; }
	virtual size_t      GetSizeOfImmutableDataForAllAllocatedNodes() const override { return 4; }
	virtual size_t      GetSizeOfRuntimeDataForAllAllocatedNodes() const override { return 4; }
	virtual void*       AllocateRuntimeData(const BehaviorTree::RuntimeDataID runtimeDataID) override { return nullptr; }
	virtual void*       GetRuntimeData(const BehaviorTree::RuntimeDataID runtimeDataID) const override { return nullptr; }
	virtual void        FreeRuntimeData(const BehaviorTree::RuntimeDataID runtimeDataID) override { }
	virtual void        SetNodeFactory(BehaviorTree::INodeFactory* nodeFactory) override { m_pNodeFactory = nodeFactory; }
	// ~INodeCreator

private:
	IManagedNodeCreator* m_pCreator;
	char m_szTypeName[64];
	BehaviorTree::INodeFactory* m_pNodeFactory;
};

class CMonoRuntime
	: public IMonoRuntime
{
public:
	CMonoRuntime();
	virtual ~CMonoRuntime() override;

	// IMonoRuntime
	virtual bool                        Initialize();
	virtual std::shared_ptr<ICryPlugin> LoadBinary(const char* szBinaryPath);

	virtual void                        Update(int updateFlags, int nPauseMode);

	virtual void                        RegisterListener(IMonoListener* pListener)   { m_listeners.Add(pListener); }
	virtual void                        UnregisterListener(IMonoListener* pListener) { m_listeners.Remove(pListener); }

	virtual IMonoDomain*                GetRootDomain() override;
	virtual IMonoDomain*                GetActiveDomain() override;
	virtual IMonoDomain*                CreateDomain(char* name, bool bActivate = false) override;

	virtual IMonoAssembly*              GetCryCommonLibrary() const override;
	virtual IMonoAssembly*              GetCryCoreLibrary() const override;

	virtual void                        RegisterManagedActor(const char* className) override;

	virtual void                        RegisterManagedNodeCreator(const char* szClassName, IManagedNodeCreator* pCreator) override;
	// ~IMonoRuntime

	CMonoDomain* FindDomainByHandle(MonoDomain* pDomain);

	CAppDomain*  LaunchPluginDomain();
	CAppDomain*  GetPluginDomain() const { return m_pPluginDomain; }

	void         HandleException(MonoObject* pException);

	void ReloadPluginDomain();

private:
	static void MonoLogCallback(const char* szLogDomain, const char* szLogLevel, const char* szMessage, mono_bool is_fatal, void* pUserData);
	static void MonoPrintCallback(const char* szMessage, mono_bool is_stdout);
	static void MonoPrintErrorCallback(const char* szMessage, mono_bool is_stdout);

	static MonoAssembly* MonoAssemblySearchCallback(MonoAssemblyName* pAssemblyName, void* pUserData);

private:
	typedef std::unordered_map<MonoDomain*, CMonoDomain*> TDomainLookupMap;
	typedef std::vector<CManagedNodeCreatorProxy*> TNodeCreators;
	TDomainLookupMap         m_domainLookupMap;
	TNodeCreators            m_nodeCreators;

	CRootMonoDomain*         m_pRootDomain;
	CAppDomain*              m_pPluginDomain;

	CMonoLibrary*            m_pLibCommon;
	CMonoLibrary*            m_pLibCore;

	MonoListeners            m_listeners;

	bool                     m_bInitializedManagedEnvironment;
};