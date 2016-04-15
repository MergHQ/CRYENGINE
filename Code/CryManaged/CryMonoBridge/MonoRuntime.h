// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoRuntime.h>
#include <CrySystem/IConsole.h>
#include <CryCore/Containers/CryListenerSet.h>
#include <mono/jit/jit.h>
#include <vector>

typedef std::vector<IMonoLibrary*> Libraries;
typedef CListenerSet<IMonoListener*> TMonoListeners;

class CMonoRuntime : public IMonoRuntime
{
public:
	CMonoRuntime (const char* sProjectDllDir);
	virtual ~CMonoRuntime() override;
	
	virtual void Initialize(EMonoLogLevel logLevel);
	virtual void LoadGame();
	virtual void UnloadGame();
	virtual void RegisterListener(IMonoListener* pListener) { m_listeners.Add(pListener); }
	virtual void UnregisterListener(IMonoListener* pListener) { m_listeners.Remove(pListener); }
	virtual void Update(int updateFlags, int nPauseMode)
	{
		for (TMonoListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
			notifier->OnUpdate(updateFlags, nPauseMode);
	}
	virtual EMonoLogLevel GetLogLevel() { return m_logLevel; }
	virtual IMonoLibraryIt* GetLibraryIterator();
	virtual const char* GetProjectDllDir() { return m_sProjectDllDir; }
private:
	template<class T> static int ReadAll(string fileName, T** data);
	static void MonoLogCallback(const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data);
	static void MonoPrintCallback(const char *string, mono_bool is_stdout);
	static void MonoPrintErrorCallback(const char *string, mono_bool is_stdout);
	static void MonoLoadHook(MonoAssembly *assembly, void* user_data);
	static MonoAssembly* MonoSearchHook(MonoAssemblyName *aname, void* user_data);
	static MonoAssembly* MonoPreLoadHook(MonoAssemblyName *aname, char **assemblies_path, void* user_data);
	
	static void CMD_ReloadGame(IConsoleCmdArgs* args);

	IMonoLibrary* LoadLibrary(const char* name);
	IMonoLibrary* GetLibrary(MonoAssembly* assembly);

private:
	friend class CMonoLibraryIt;
	Libraries		m_loadedLibraries;
	TMonoListeners	m_listeners;
	MonoDomain*		m_domain;
	MonoDomain*		m_gameDomain;
	IMonoLibrary*	m_gameLibrary;
	bool			m_bSearchOpen;
	EMonoLogLevel	m_logLevel;
	char			m_sBasePath[_MAX_PATH];
	char			m_sProjectDllDir[_MAX_PATH];
};