// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <mono/jit/jit.h>
#include <CrySystem/IConsole.h>
#include <CryMono/IMonoRuntime.h>
#include <CryCore/Containers/CryListenerSet.h>

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
	TDomainLookupMap         m_domainLookupMap;

	CRootMonoDomain*         m_pRootDomain;
	CAppDomain*              m_pPluginDomain;

	CMonoLibrary*            m_pLibCommon;
	CMonoLibrary*            m_pLibCore;

	MonoListeners            m_listeners;

	bool                     m_bInitializedManagedEnvironment;
};
