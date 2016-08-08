// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <mono/jit/jit.h>
#include <CrySystem/IConsole.h>
#include <CryMono/IMonoRuntime.h>
#include <CryCore/Containers/CryListenerSet.h>

struct SMonoDomainHandlerLibrary;
struct SExplicitPluginContainer
{
	SExplicitPluginContainer(ICryEngineBasePlugin* pBasePlugin, IMonoLibrary* pLibrary, bool bIsExcluded, string binaryPath)
		: m_pPlugin(pBasePlugin)
		, m_pLibrary(pLibrary)
		, m_bIsExcluded(bIsExcluded)
		, m_binaryPath(binaryPath)
	{}

	ICryEngineBasePlugin* m_pPlugin;
	IMonoLibrary*         m_pLibrary;
	bool                  m_bIsExcluded;
	string                m_binaryPath;
};

typedef std::vector<IMonoLibrary*>            Libraries;
typedef CListenerSet<IMonoListener*>          MonoListeners;
typedef std::vector<SExplicitPluginContainer> ExplicitLibraries;

class CMonoRuntime : public IMonoRuntime
{
public:
	CMonoRuntime(const char* szProjectDllDir);
	virtual ~CMonoRuntime() override;

	virtual void                  Initialize(EMonoLogLevel logLevel);
	virtual bool                  LoadBinary(const char* szBinaryPath);
	virtual bool                  UnloadBinary(const char* szBinaryPath);

	virtual void                  Update(int updateFlags, int nPauseMode);
	virtual ICryEngineBasePlugin* GetPlugin(const char* szBinaryPath) const;
	virtual bool                  RunMethod(const ICryEngineBasePlugin* pPlugin, const char* szMethodName) const;

	virtual void                  RegisterListener(IMonoListener* pListener)   { m_listeners.Add(pListener); }
	virtual void                  UnregisterListener(IMonoListener* pListener) { m_listeners.Remove(pListener); }
	virtual EMonoLogLevel         GetLogLevel()                                { return m_logLevel; }

public:
	IMonoLibrary* GetCommon() { return m_pLibCommon; }
	IMonoLibrary* GetCore()   { return m_pLibCore; }

private:
	template<class T> static int ReadAll(string fileName, T** data);
	static void                  MonoLogCallback(const char* szLogDomain, const char* szLogLevel, const char* szMessage, mono_bool is_fatal, void* pUserData);
	static void                  MonoPrintCallback(const char* szMessage, mono_bool is_stdout);
	static void                  MonoPrintErrorCallback(const char* szMessage, mono_bool is_stdout);
	static void                  MonoLoadHook(MonoAssembly* pAssembly, void* pUserData);
	static MonoAssembly*         MonoSearchHook(MonoAssemblyName* pAssemblyName, void* pUserData);

	SMonoDomainHandlerLibrary*   LoadDomainHandlerLibrary(const char* szLibraryName);
	IMonoLibrary*                LoadLibrary(const char* szLibraryName);
	IMonoLibrary*                GetLibrary(MonoAssembly* pAssembly);

	bool                         LaunchPluginDomain();
	bool                         ReloadPluginDomain();
	bool                         StopPluginDomain();

	bool                         ExcludePlugin(const char* szPluginName);
	const char*                  TryGetPlugin(MonoDomain* pDomain, const char* szAssembly);

private:
	EMonoLogLevel              m_logLevel;
	IMonoLibrary*              m_pLibCommon;
	IMonoLibrary*              m_pLibCore;
	SMonoDomainHandlerLibrary* m_pDomainHandler;
	MonoDomain*                m_pDomainGlobal;
	MonoDomain*                m_pDomainPlugins;
	MonoListeners              m_listeners;
	bool                       m_bSearchOpen;
	char                       m_sBaseDirectory[_MAX_PATH];
	char                       m_sProjectDllDir[_MAX_PATH];
	ExplicitLibraries          m_explicitLibraries;
	Libraries                  m_loadedLibraries;
};
