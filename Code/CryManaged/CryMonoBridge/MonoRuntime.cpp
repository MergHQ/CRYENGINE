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

#include "NativeToManagedInterfaces/Entity.h"
#include "NativeToManagedInterfaces/Console.h"

#include <CrySystem/ILog.h>
#include <CryAISystem/IAISystem.h>
#include <CrySystem/IConsole.h>

#include <mono/metadata/mono-gc.h>
#include <mono/metadata/assembly.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/exception.h>

#include <CryMono/IMonoNativeToManagedInterface.h>
#include <CryInput/IHardwareMouse.h>

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
	GetMonoRuntime()->ReloadPluginDomain();
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
	gEnv->pConsole->UnregisterListener(this);

	if (m_pLibCore != nullptr)
	{
		// Get the equivalent of gEnv
		auto pEngineClass = m_pLibCore->GetTemporaryClass("CryEngine", "Engine");
		CRY_ASSERT(pEngineClass != nullptr);
	
		// Call the static Shutdown function
		pEngineClass->FindMethod("OnEngineShutdown")->Invoke();
	}

	// Root domain HAS to be deleted last, its destructor shuts down the entire runtime!
	m_domainLookupMap.clear();
	m_pRootDomain.reset();

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

	// Find the Mono configuration directory
	char engineRoot[_MAX_PATH];
	CryFindEngineRootFolder(_MAX_PATH, engineRoot);

	const char* szMonoDirectoryParent = "bin\\common";

	char sMonoLib[_MAX_PATH];
	sprintf_s(sMonoLib, "%s\\%s\\Mono\\lib", engineRoot, szMonoDirectoryParent);
	char sMonoEtc[_MAX_PATH];
	sprintf_s(sMonoEtc, "%s\\%s\\Mono\\etc", engineRoot, szMonoDirectoryParent);

	if (!gEnv->pCryPak->IsFileExist(sMonoLib) || !gEnv->pCryPak->IsFileExist(sMonoEtc))
	{
		CryLogAlways("Failed to initialize Mono runtime, Mono directory was not found or incomplete in %s directory", szMonoDirectoryParent);
		delete this;

		return false;
	}

	mono_set_dirs(sMonoLib, sMonoEtc);

	mono_trace_set_level_string(s_monoLogLevels[0]);

	mono_trace_set_log_handler(MonoLogCallback, this);
	mono_trace_set_print_handler(MonoPrintCallback);
	mono_trace_set_printerr_handler(MonoPrintErrorCallback);

	m_pRootDomain = std::make_shared<CRootMonoDomain>();
	m_pRootDomain->Initialize();

	m_domainLookupMap.emplace(std::make_pair((MonoDomain*)m_pRootDomain->GetHandle(), m_pRootDomain));

	RegisterInternalInterfaces();

	gEnv->pConsole->RegisterListener(this, "MonoRuntime::ManagedConsoleCommandListener");

	CryLog("[Mono] Initialization done.");
	return true;
}

std::shared_ptr<ICryPlugin> CMonoRuntime::LoadBinary(const char* szBinaryPath)
{
	return std::make_shared<CManagedPlugin>(szBinaryPath);
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

IMonoDomain* CMonoRuntime::GetRootDomain()
{
	return m_pRootDomain.get();
}

IMonoDomain* CMonoRuntime::GetActiveDomain()
{
	MonoDomain* pActiveMonoDomain = mono_domain_get();
	
	if (CMonoDomain* pDomain = FindDomainByHandle(pActiveMonoDomain))
	{
		return pDomain;
	}

	auto pair = m_domainLookupMap.emplace(std::make_pair(pActiveMonoDomain, std::make_shared<CAppDomain>(pActiveMonoDomain)));
	return pair.first->second.get();
}

IMonoDomain* CMonoRuntime::CreateDomain(char* name, bool bActivate)
{
	auto pDomain = std::make_shared<CAppDomain>(name, bActivate);
	auto pair = m_domainLookupMap.emplace(std::make_pair((MonoDomain*)pDomain->GetHandle(), pDomain));

	return pDomain.get();
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
	IGameObjectExtension* Create(IEntity *pEntity)
	{
		return pEntity->CreateComponentClass<T>();
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

	static CEntityComponentCreator<CManagedActor> actorCreator;
	IEntityClassRegistry::SEntityClassDesc clsDesc;
	clsDesc.sName = actorClassName;
	gEnv->pGameFramework->GetIGameObjectSystem()->RegisterExtension(clsDesc.sName, &actorCreator, &clsDesc);
}

void CMonoRuntime::RegisterManagedNodeCreator(const char* szClassName, IManagedNodeCreator* pCreator)
{
	BehaviorTree::IBehaviorTreeManager& manager = *gEnv->pAISystem->GetIBehaviorTreeManager();
	m_nodeCreators.emplace_back(std::make_shared<CManagedNodeCreatorProxy>(szClassName, pCreator));
	manager.GetNodeFactory().RegisterNodeCreator(m_nodeCreators.back().get());
}

void CMonoRuntime::RegisterNativeToManagedInterface(IMonoNativeToManagedInterface& interface)
{
	string functionNamePrefix;
	functionNamePrefix.Format("%s.%s::", interface.GetNamespace(), interface.GetClassName());

	auto registerInternalCall = [functionNamePrefix](const void* pMethod, const char* szMethodName)
	{
		string methodName = functionNamePrefix + szMethodName;

		mono_add_internal_call(methodName, pMethod);
	};

	interface.RegisterFunctions(registerInternalCall);
}

CMonoDomain* CMonoRuntime::FindDomainByHandle(MonoDomain* pDomain)
{
	auto domainIt = m_domainLookupMap.find(pDomain);
	if (domainIt == m_domainLookupMap.end())
	{
		domainIt = m_domainLookupMap.emplace(std::make_pair(pDomain, std::make_shared<CAppDomain>(pDomain))).first;
	}

	return domainIt->second.get();
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
			m_pPluginDomain = nullptr;
			return nullptr;
		}

		libraryPath = PathUtil::Make(executableFolder, "CryEngine.Core");
		m_pLibCore = m_pPluginDomain->LoadLibrary(libraryPath);
		if (m_pLibCore == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load managed core library %s", libraryPath.c_str());
			m_pPluginDomain = nullptr;
			return nullptr;
		}

		// Get the equivalent of gEnv
		auto pEngineClass = m_pLibCore->GetTemporaryClass("CryEngine", "Engine");
		CRY_ASSERT(pEngineClass != nullptr);

		// Call the static Initialize function
		pEngineClass->FindMethod("OnEngineStart")->Invoke();

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
	pEngineClass->FindMethod("OnUnloadStart")->Invoke();

	if (m_pPluginDomain->Reload())
	{
		static_cast<CMonoClass*>(pEngineClass.get())->ReloadClass();

		// Notify the framework so that internal listeners etc. can be added again.
		pEngineClass->FindMethod("OnReloadDone")->Invoke();
	}
}

void CMonoRuntime::RegisterInternalInterfaces()
{
	CManagedEntityInterface entityInterface;
	RegisterNativeToManagedInterface(entityInterface);

	CConsoleCommandInterface consoleCommandInterface;
	RegisterNativeToManagedInterface(consoleCommandInterface);
}

void CMonoRuntime::OnManagedConsoleCommandEvent(const char* commandName, IConsoleCmdArgs* consoleCommandArguments)
{
	gEnv->pLog->Log("[Mono] %s", "OnManagedConsoleCommandEvent invoked");
	InvokeManagedConsoleCommandNotification(commandName, consoleCommandArguments);
}

void CMonoRuntime::InvokeManagedConsoleCommandNotification(const char* commandName, IConsoleCmdArgs* commandArguments)
{
	IMonoClass* classConsoleCommandArgumentHolder = m_pLibCore->GetClass("CryEngine", "ConsoleCommandArgumentsHolder");
	void* constructorArg[1];
	int noArguments = commandArguments->GetArgCount();
	constructorArg[0] = &noArguments;
	std::shared_ptr<IMonoObject> consoleCommandArgumentHolderInstance = classConsoleCommandArgumentHolder->CreateInstance(constructorArg, 1);

	void* methodArg[2];
	for (int j = 0; j < noArguments; ++j)
	{
		methodArg[0] = &j;
		methodArg[1] = m_pRootDomain->CreateManagedString(commandArguments->GetArg(j));
		consoleCommandArgumentHolderInstance->GetClass()->FindMethod("SetArgument", 2)->Invoke(consoleCommandArgumentHolderInstance.get(), methodArg);
	}

	void* methodArg2[3];
	methodArg2[0] = m_pRootDomain->CreateManagedString(commandName);
	methodArg2[1] = &noArguments;
	methodArg2[2] = consoleCommandArgumentHolderInstance->GetHandle();
	IMonoClass* classConsoleCommand = m_pLibCore->GetClass("CryEngine", "ConsoleCommand");
	classConsoleCommand->FindMethod("NotifyManagedConsoleCommand", 3)->Invoke(nullptr, methodArg2);
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

	gEnv->pHardwareMouse->UseSystemCursor(true);
	
	void* args[1];
	args[0] = pException;

	auto pExceptionHandlerClass = m_pLibCore->GetTemporaryClass("CryEngine.Debugging", "ExceptionHandler");
	CRY_ASSERT(pExceptionHandlerClass != nullptr);

	// Call the static Initialize function
	pExceptionHandlerClass->FindMethod("Display", 1)->Invoke(nullptr, args);
}