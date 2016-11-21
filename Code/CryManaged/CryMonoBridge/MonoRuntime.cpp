// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoRuntime.h"

#include "Wrappers/RootMonoDomain.h"
#include "Wrappers/AppDomain.h"
#include "Wrappers/MonoLibrary.h"
#include "Wrappers/MonoClass.h"
#include "Wrappers/MonoObject.h"

#include "ManagedPlugin.h"

#include "NativeComponents/Actor.h"
#include "NativeComponents/GameRules.h"

#include <CrySystem/ILog.h>
#include <CryAISystem/IAISystem.h>

#include <mono/metadata/mono-gc.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/exception.h>

#if CRY_PLATFORM_WINDOWS
	#pragma comment(lib, "mono-2.0.lib")
#endif

static const char* s_monoLogLevels[] =
{
	NULL,
	"error",
	"critical",
	"warning",
	"message",
	"info",
	"debug"
};

static IMiniLog::ELogType s_monoToEngineLevels[] =
{
	IMiniLog::eAlways,
	IMiniLog::eErrorAlways,
	IMiniLog::eError,
	IMiniLog::eWarning,
	IMiniLog::eMessage,
	IMiniLog::eMessage,
	IMiniLog::eComment
};

void OnReloadRequested(IConsoleCmdArgs *pArgs)
{
	static_cast<CMonoRuntime*>(gEnv->pMonoRuntime)->ReloadPluginDomain();
}

CMonoRuntime::CMonoRuntime()
	: m_pLibCommon(nullptr)
	, m_pLibCore(nullptr)
	, m_pRootDomain(nullptr)
	, m_pPluginDomain(nullptr)
	, m_listeners(5)
{
	REGISTER_COMMAND("mono_reload", OnReloadRequested, VF_NULL, "Used to reload all mono plug-ins");
}

CMonoRuntime::~CMonoRuntime()
{
	if (m_pLibCore != nullptr)
	{
		// Get the equivalent of gEnv
		auto pEngineClass = m_pLibCore->GetTemporaryClass("CryEngine", "Engine");
		CRY_ASSERT(pEngineClass != nullptr);

		// Call the static Shutdown function
		pEngineClass->InvokeMethod("OnEngineShutdown");
	}

	for (auto it = m_domainLookupMap.begin(); it != m_domainLookupMap.end(); ++it)
	{
		// Root domain HAS to be deleted last, its destructor shuts down the entire runtime!
		if (it->second != m_pRootDomain)
		{
			it->second->Release();
			it->second = nullptr;
		}
	}

	SAFE_DELETE(m_pRootDomain);

	gEnv->pMonoRuntime = nullptr;
}

bool CMonoRuntime::Initialize()
{
	CryLog("[Mono] Initialize Mono Runtime . . . ");

#ifndef _RELEASE
	mono_debug_init(MONO_DEBUG_FORMAT_MONO);
	char* options[] = {
		"--soft-breakpoints",
		"--debugger-agent=transport=dt_socket,address=127.0.0.1:17615,embedding=1,server=y,suspend=n"
	};
	mono_jit_parse_options(sizeof(options) / sizeof(char*), options);
#endif

	char engineRoot[_MAX_PATH];
	CryFindEngineRootFolder(_MAX_PATH, engineRoot);

	char sMonoLib[_MAX_PATH];
	sprintf_s(sMonoLib, "%s\\Engine\\mono\\lib", engineRoot);
	char sMonoEtc[_MAX_PATH];
	sprintf_s(sMonoEtc, "%s\\Engine\\mono\\etc", engineRoot);

	if (!gEnv->pCryPak->IsFileExist(sMonoLib) || !gEnv->pCryPak->IsFileExist(sMonoEtc))
	{
		CryLogAlways("Failed to initialize Mono runtime, Mono directory was not found or incomplete in Engine directory");
		delete this;

		return false;
	}

	mono_set_dirs(sMonoLib, sMonoEtc);

	mono_trace_set_level_string(s_monoLogLevels[0]);

	mono_trace_set_log_handler(MonoLogCallback, this);
	mono_trace_set_print_handler(MonoPrintCallback);
	mono_trace_set_printerr_handler(MonoPrintErrorCallback);

	mono_install_assembly_search_hook(MonoAssemblySearchCallback, (void *)1);
	mono_install_assembly_refonly_search_hook(MonoAssemblySearchCallback, (void *)0);

	m_pRootDomain = new CRootMonoDomain();
	m_pRootDomain->Initialize();

	m_domainLookupMap.insert(TDomainLookupMap::value_type(m_pRootDomain->GetHandle(), m_pRootDomain));

	CryLog("[Mono] Initialization done.");
	return true;
}

// This function is required seeing as ICryPak::IsFileExist only works for files in the game directory
// Should be replaced if that function is fixed.
bool DoesFileExist(const char* path)
{
	if (auto pHandle = gEnv->pCryPak->FOpen(path, "rb", ICryPak::FLAGS_PATH_REAL))
	{
		gEnv->pCryPak->FClose(pHandle);
		return true;
	}

	return false;
}

std::shared_ptr<ICryPlugin> CMonoRuntime::LoadBinary(const char* szBinaryPath)
{
	if (!DoesFileExist(szBinaryPath))
	{
		return nullptr;
	}

	return std::shared_ptr<ICryPlugin>(new CManagedPlugin(szBinaryPath));
}

void CMonoRuntime::Update(int updateFlags, int nPauseMode)
{
	for (MonoListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnUpdate(updateFlags, nPauseMode);
	}
}

void CMonoRuntime::MonoLogCallback(const char* szLogDomain, const char* szLogLevel, const char* szMessage, mono_bool is_fatal, void* pUserData)
{
	IMiniLog::ELogType engineLvl = IMiniLog::eComment;
	if (szLogLevel)
		for (int i = 1; i < CRY_ARRAY_COUNT(s_monoLogLevels); i++)
			if (_stricmp(s_monoLogLevels[i], szLogLevel) == 0)
			{
				engineLvl = s_monoToEngineLevels[i];
				break;
			}

	gEnv->pLog->LogWithType(engineLvl, "[Mono][%s][%s] %s", szLogDomain, szLogLevel, szMessage);

#if CRY_PLATFORM_WINDOWS && !defined(RELEASE)
	if (IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif
}

void CMonoRuntime::MonoPrintCallback(const char* szMessage, mono_bool is_stdout)
{
	gEnv->pLog->Log("[Mono] %s", szMessage);
}

void CMonoRuntime::MonoPrintErrorCallback(const char* szMessage, mono_bool is_stdout)
{
	gEnv->pLog->LogError("[Mono] %s", szMessage);

#if CRY_PLATFORM_WINDOWS && !defined(RELEASE)
	if (IsDebuggerPresent())
	{
		__debugbreak();
	}
#endif
}

MonoAssembly* CMonoRuntime::MonoAssemblySearchCallback(MonoAssemblyName* pAssemblyName, void* pUserData)
{
	bool bRefOnly = ((int)pUserData == 0);

	auto* pDomain = static_cast<CMonoRuntime*>(gEnv->pMonoRuntime)->GetActiveDomain();

	const char* assemblyName = mono_assembly_name_get_name(pAssemblyName);

	if (auto* pLibrary = static_cast<CMonoDomain*>(pDomain)->LoadLibrary(assemblyName, bRefOnly))
	{
		return pLibrary->GetAssembly();
	}

	return nullptr;
}

IMonoDomain* CMonoRuntime::GetRootDomain()
{
	return m_pRootDomain;
}

IMonoDomain* CMonoRuntime::GetActiveDomain()
{
	MonoDomain* pActiveMonoDomain = mono_domain_get();
	auto* pDomain = FindDomainByHandle(pActiveMonoDomain);
	if (pDomain != nullptr)
	{
		return pDomain;
	}

	pDomain = new CAppDomain(pActiveMonoDomain);
	m_domainLookupMap.insert(TDomainLookupMap::value_type(pActiveMonoDomain, pDomain));

	return pDomain;
}

IMonoDomain* CMonoRuntime::CreateDomain(char* name, bool bActivate)
{
	auto* pAppDomain = new CAppDomain(name, bActivate);
	m_domainLookupMap.insert(TDomainLookupMap::value_type(pAppDomain->GetHandle(), pAppDomain));

	return pAppDomain;
}

IMonoAssembly* CMonoRuntime::GetCryCommonLibrary() const
{
	return m_pLibCommon; 
}

IMonoAssembly* CMonoRuntime::GetCryCoreLibrary() const
{
	return m_pLibCore; 
}

template<class T>
struct CEntityComponentCreator : public IGameObjectExtensionCreatorBase
{
	IGameObjectExtensionPtr Create()
	{
		return ComponentCreate_DeleteWithRelease<T>();
	}

	void GetGameObjectExtensionRMIData(void** ppRMI, size_t* nCount)
	{
		*ppRMI = nullptr;
		*nCount = 0;
	}
};

void CMonoRuntime::RegisterManagedActor(const char* actorClassName)
{
	if (gEnv->pEntitySystem->GetClassRegistry()->FindClass(actorClassName) != nullptr)
	{
		//HandleException((MonoObject*)mono_get_exception_argument("className", "Tried to register actor twice!"));
		return;
	}

	const char* rulesName = "ManagedGameRules";

	if (gEnv->pEntitySystem->GetClassRegistry()->FindClass(rulesName) != nullptr)
	{
		HandleException((MonoObject*)mono_get_exception_argument("className", "Tried to register actor twice!"));
		return;
	}

	IEntityClassRegistry::SEntityClassDesc clsDesc;
	
	static CEntityComponentCreator<CManagedGameRules> gameRulesCreator;
	clsDesc.sName = rulesName;
	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(clsDesc.sName, &gameRulesCreator, &clsDesc);

	CManagedGameRules::s_actorClassName = actorClassName;
	gEnv->pGameFramework->GetIGameRulesSystem()->RegisterGameRules(rulesName, rulesName, false);

	gEnv->pConsole->GetCVar("sv_gamerulesdefault")->Set(rulesName);
	gEnv->pConsole->GetCVar("sv_gamerules")->Set(rulesName);

	static CEntityComponentCreator<CManagedActor> actorCreator;
	clsDesc.sName = actorClassName;
	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(clsDesc.sName, &actorCreator, &clsDesc);
}

void CMonoRuntime::RegisterManagedNodeCreator(const char* szClassName, IManagedNodeCreator* pCreator)
{
	BehaviorTree::IBehaviorTreeManager& manager = *gEnv->pAISystem->GetIBehaviorTreeManager();
	CManagedNodeCreatorProxy* pProxy = new CManagedNodeCreatorProxy(szClassName, pCreator);
	manager.GetNodeFactory().RegisterNodeCreator(pProxy);
	m_nodeCreators.push_back(pProxy);
}

CMonoDomain* CMonoRuntime::FindDomainByHandle(MonoDomain* pDomain)
{
	auto domainIt = m_domainLookupMap.find(pDomain);
	if (domainIt == m_domainLookupMap.end())
	{
		domainIt = m_domainLookupMap.insert(TDomainLookupMap::value_type(pDomain, new CAppDomain(pDomain))).first;
	}

	return domainIt->second;
}

CAppDomain* CMonoRuntime::LaunchPluginDomain()
{
	if (m_pPluginDomain != nullptr)
		return m_pPluginDomain;

	// Create the plugin domain and activate it
	if (m_pPluginDomain = static_cast<CAppDomain*>(CreateDomain("CryEngine.Plugins", true)))
	{
		char executableFolder[_MAX_PATH];
		CryGetExecutableFolder(_MAX_PATH, executableFolder);

		string libraryPath = PathUtil::Make(executableFolder, "CryEngine.Common");
		m_pLibCommon = m_pPluginDomain->LoadLibrary(libraryPath);
		if (m_pLibCommon == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load managed common library %s", libraryPath.c_str());

			delete m_pPluginDomain;
			m_pPluginDomain = nullptr;

			return nullptr;
		}

		libraryPath = PathUtil::Make(executableFolder, "CryEngine.Core");
		m_pLibCore = m_pPluginDomain->LoadLibrary(libraryPath);
		if (m_pLibCore == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load managed core library %s", libraryPath.c_str());

			delete m_pPluginDomain;
			m_pPluginDomain = nullptr;

			return nullptr;
		}

		// Get the equivalent of gEnv
		auto pEngineClass = m_pLibCore->GetTemporaryClass("CryEngine", "Engine");
		CRY_ASSERT(pEngineClass != nullptr);

		// Call the static Initialize function
		pEngineClass->InvokeMethod("OnEngineStart");

		return m_pPluginDomain;
	}

	return nullptr;
}

void CMonoRuntime::ReloadPluginDomain()
{
	// Trigger removal of C++ listeners, since the memory they belong to will be invalid shortly
	auto pEngineClass = m_pLibCore->GetTemporaryClass("CryEngine", "Engine");
	CRY_ASSERT(pEngineClass != nullptr);

	// Call the static Shutdown function
	pEngineClass->InvokeMethod("OnUnloadStart");

	if (m_pPluginDomain->Reload())
	{
		static_cast<CMonoClass*>(pEngineClass.get())->ReloadClass();

		// Notify the framework so that internal listeners etc. can be added again.
		pEngineClass->InvokeMethod("OnReloadDone");
	}
}

void CMonoRuntime::HandleException(MonoObject* pException)
{
#if CRY_PLATFORM_WINDOWS && !defined(RELEASE)
	if (IsDebuggerPresent())
	{
		if (MonoString* pString = mono_object_to_string(pException, nullptr))
		{
			const char* errorMessage = mono_string_to_utf8(pString);
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Handled managed exception with debugger attached:\n%s", errorMessage);

			__debugbreak();
		}
	}
#endif

	void* args[1];
	args[0] = pException;

	auto pExceptionHandlerClass = m_pLibCore->GetTemporaryClass("CryEngine.Debugging", "ExceptionHandler");
	CRY_ASSERT(pExceptionHandlerClass != nullptr);

	// Call the static Initialize function
	pExceptionHandlerClass->InvokeMethod("Display", nullptr, args, 1);
}