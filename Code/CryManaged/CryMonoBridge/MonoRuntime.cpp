// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoRuntime.h"
#include "MonoLibrary.h"

#include <CrySystem/ILog.h>

#include <mono/metadata/mono-gc.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>

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

CMonoRuntime::CMonoRuntime(const char* szProjectDllDir)
	: m_logLevel(eMLL_NULL)
	, m_pLibCommon(nullptr)
	, m_pLibCore(nullptr)
	, m_pDomainGlobal(nullptr)
	, m_pDomainPlugins(nullptr)
	, m_listeners(5)
	, m_bSearchOpen(false)
{
	cry_strcpy(m_sProjectDllDir, szProjectDllDir);
}

CMonoRuntime::~CMonoRuntime()
{
	StopPluginDomain();

	mono_runtime_cleanup(m_pDomainPlugins);
	mono_jit_cleanup(m_pDomainGlobal);
}

void CMonoRuntime::Initialize(EMonoLogLevel logLevel)
{
	CryLog("[Mono] Initialize Mono Runtime . . . ");

	CryGetExecutableFolder(CRY_ARRAY_COUNT(m_sBaseDirectory), m_sBaseDirectory);

#ifndef _RELEASE
	mono_debug_init(MONO_DEBUG_FORMAT_MONO);
	char* options[] = {
		"--soft-breakpoints",
		"--debugger-agent=transport=dt_socket,address=127.0.0.1:17615,embedding=1,server=y,suspend=n"
	};
	mono_jit_parse_options(sizeof(options) / sizeof(char*), options);
#endif

	char sMonoLib[_MAX_PATH];
	sprintf_s(sMonoLib, "%smono\\lib", m_sBaseDirectory);
	char sMonoEtc[_MAX_PATH];
	sprintf_s(sMonoEtc, "%smono\\etc", m_sBaseDirectory);

	mono_set_dirs(sMonoLib, sMonoEtc);
	m_logLevel = logLevel;
	mono_trace_set_level_string(s_monoLogLevels[m_logLevel]);

	mono_trace_set_log_handler(MonoLogCallback, this);
	mono_trace_set_print_handler(MonoPrintCallback);
	mono_trace_set_printerr_handler(MonoPrintErrorCallback);
	mono_install_assembly_load_hook(MonoLoadHook, this);
	mono_install_assembly_search_hook(MonoSearchHook, this);
	mono_install_assembly_refonly_search_hook(MonoSearchHook, this);

	m_pDomainGlobal = mono_jit_init_version("CryEngine", "v4.0.30319");
	if (m_pDomainGlobal)
	{
		CryLog("[Mono] Initialization done.");
	}
	else
	{
		gEnv->pLog->LogError("[Mono] Could not create CryEngine domain");
		CryLog("[Mono] Initialization failed!");
	}
}

bool CMonoRuntime::LoadBinary(const char* szBinaryPath)
{
	if (!m_pDomainPlugins)
	{
		LaunchPluginDomain();
	}

	if (!m_pDomainPlugins) // Invalid app domain
	{
		CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "No valid app domain for mono assembly found! (%s)", szBinaryPath);
		return false;
	}

	string binaryFile = PathUtil::GetFile(szBinaryPath);
	string binaryPath = PathUtil::GetPathWithoutFilename(szBinaryPath);
	cry_strcpy(m_sProjectDllDir, binaryPath.c_str());

	IMonoLibrary* pLibrary = LoadLibrary(binaryFile.c_str());
	if (!pLibrary)
	{
		gEnv->pLog->LogError("[Mono] could not load plugin library!");
		return false;
	}

	// Make sure debugger is connected.
	CrySleep(100);

	ICryEnginePlugin* pPlugin = pLibrary->Initialize(m_pDomainPlugins);
	if (pPlugin != nullptr)
	{
		bool bExist = false;
		for (ExplicitLibraries::iterator it = m_explicitLibraries.begin(); it != m_explicitLibraries.end(); ++it)
		{
			if (!stricmp(it->m_binaryPath.c_str(), szBinaryPath))
			{
				it->m_pPlugin = pPlugin;
				it->m_pLibrary = pLibrary;
				it->m_bIsExcluded = false;
				bExist = true;
			}
		}

		if (!bExist)
		{
			m_explicitLibraries.push_back(SExplicitPluginContainer(pPlugin, pLibrary, false, string(szBinaryPath)));
		}

		// DBG
		gEnv->pLog->Log("[Mono] Loaded Libraries:");
		for (Libraries::const_iterator it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
		{
			if ((*it)->IsInMemory())
				gEnv->pLog->Log("[Mono]   %s (In Memory)", (*it)->GetImageName());
			else
				gEnv->pLog->Log("[Mono]   %s", (*it)->GetImageName());
		}
		//// ~DBG

		return true;
	}
	else
	{
		// TODO: safely unload assembly again
		gEnv->pLog->LogError("[Mono] could not initialize plugin '%s'!", szBinaryPath);
	}

	return false;
}

bool CMonoRuntime::UnloadBinary(const char* szBinaryPath)
{
	if (!ExcludePlugin(szBinaryPath))
	{
		CryLog("Can't find loaded instance of c# plugin '%s', skipping unload...", szBinaryPath);
		return false;
	}

	return ReloadPluginDomain();
}

void CMonoRuntime::Update(int updateFlags, int nPauseMode)
{
	for (MonoListeners::Notifier notifier(m_listeners); notifier.IsValid(); notifier.Next())
	{
		notifier->OnUpdate(updateFlags, nPauseMode);
	}
}

ICryEnginePlugin* CMonoRuntime::GetPlugin(const char* szBinaryPath) const
{
	for (ExplicitLibraries::const_iterator it = m_explicitLibraries.begin(); it != m_explicitLibraries.end(); ++it)
	{
		if (!strcmp(it->m_binaryPath, szBinaryPath))
		{
			return it->m_pPlugin;
		}
	}

	return nullptr;
}

bool CMonoRuntime::RunMethod(const ICryEnginePlugin* pPlugin, const char* szMethodName) const
{
	for (ExplicitLibraries::const_iterator itExplicit = m_explicitLibraries.begin(); itExplicit != m_explicitLibraries.end(); ++itExplicit)
	{
		if (itExplicit->m_pPlugin == pPlugin && itExplicit->m_pLibrary)
		{
			return itExplicit->m_pLibrary->RunMethod(szMethodName);
		}
	}

	return false;
}

template<class T>
int CMonoRuntime::ReadAll(string fileName, T** data)
{
	_finddata_t fd;
	intptr_t handle = gEnv->pCryPak->FindFirst(fileName.c_str(), &fd, ICryPak::FLAGS_PATH_REAL, true);
	if (handle < 0)
		return 0;

	FILE* f = gEnv->pCryPak->FOpen(fileName, "rb", ICryPak::FLAGS_PATH_REAL);
	int size = gEnv->pCryPak->FGetSize(f);
	*data = new T[size];
	gEnv->pCryPak->FRead(*data, size, f);
	gEnv->pCryPak->FClose(f);
	return size;
}

void CMonoRuntime::MonoLogCallback(const char* szLogDomain, const char* szLogLevel, const char* szMessage, mono_bool is_fatal, void* pUserData)
{
	IMiniLog::ELogType engineLvl = IMiniLog::eComment;
	if (szLogLevel)
		for (int i = 1; i < 7; i++)
			if (_stricmp(s_monoLogLevels[i], szLogLevel) == 0)
			{
				engineLvl = s_monoToEngineLevels[i];
				break;
			}

	gEnv->pLog->LogWithType(engineLvl, "[Mono][%s][%s] %s", szLogDomain, szLogLevel, szMessage);
}

void CMonoRuntime::MonoPrintCallback(const char* szMessage, mono_bool is_stdout)
{
	gEnv->pLog->Log("[Mono] %s", szMessage);
}

void CMonoRuntime::MonoPrintErrorCallback(const char* szMessage, mono_bool is_stdout)
{
	gEnv->pLog->LogError("[Mono] %s", szMessage);
}

void CMonoRuntime::MonoLoadHook(MonoAssembly* pAssembly, void* pUserData)
{
	CMonoRuntime* pRuntime = (CMonoRuntime*)pUserData;
	if (!pRuntime)
		return;

	IMonoLibrary* pLib = pRuntime->GetLibrary(pAssembly);
	if (!pLib)
	{
		pLib = new CMonoLibrary(pAssembly);
		pRuntime->m_loadedLibraries.push_back(pLib);
	}
}

MonoAssembly* CMonoRuntime::MonoSearchHook(MonoAssemblyName* pAssemblyName, void* pUserData)
{
	CMonoRuntime* pRuntime = (CMonoRuntime*)pUserData;
	if (!pRuntime || pRuntime->m_bSearchOpen)
	{
		return nullptr;
	}

	string asmName = mono_assembly_name_get_name(pAssemblyName);

	if (strcmp("dll", PathUtil::GetExt(asmName.c_str())))
	{
		asmName += ".dll";
	}

	MonoImageOpenStatus status = MONO_IMAGE_ERROR_ERRNO;
	string fName = PathUtil::Make(pRuntime->m_sBaseDirectory, asmName.c_str());

	//Prefer project dll if available
	string fProjectName = PathUtil::Make(pRuntime->m_sProjectDllDir, asmName.c_str());
	if (gEnv->pCryPak->IsFileExist(fProjectName.c_str(), ICryPak::eFileLocation_OnDisk))
	{
		fName = fProjectName;
	}

	char* dataImg;
	int nImg = ReadAll(fName, &dataImg);
	if (nImg <= 0)
	{
		return nullptr;
	}

	pRuntime->m_bSearchOpen = true;
	MonoImage* pImg = mono_image_open_from_data_full(dataImg, nImg, false, &status, 0);

	// load debug information
#ifndef _RELEASE
	string fNameDbg = fName + ".mdb";
	mono_byte* dataDbg;
	int nDbg = ReadAll(fNameDbg, &dataDbg);
	if (nDbg > 0)
	{
		mono_debug_open_image_from_memory(pImg, dataDbg, nDbg);
	}
#endif
	// load assembly and set data
	MonoAssembly* pAsm = mono_assembly_load_from_full(pImg, asmName.c_str(), &status, 0);
	// store the memory image of the assembly, so that we can delete it, when unloading the assembly
	IMonoLibrary* pLib = pRuntime->GetLibrary(pAsm);
	if (!pLib)
	{
		pLib = new CMonoLibrary(pAsm, dataImg);
		pRuntime->m_loadedLibraries.push_back(pLib);
	}
	else
	{
		((CMonoLibrary*)pLib)->SetData(dataImg);
	}
#ifndef _RELEASE
	if (nDbg > 0)
	{
		((CMonoLibrary*)pLib)->SetDebugData(dataDbg);
	}
#endif
	pRuntime->m_bSearchOpen = false;

	return pAsm;
}

IMonoLibrary* CMonoRuntime::LoadLibrary(const char* szLibraryName)
{
	gEnv->pLog->LogWithType(IMiniLog::eMessage, "[Mono] Load Library: %s", szLibraryName);

	MonoImageOpenStatus status = MonoImageOpenStatus::MONO_IMAGE_ERROR_ERRNO;
	MonoAssemblyName* pAsmName = mono_assembly_name_new(szLibraryName);
	MonoAssembly* pAsm = mono_assembly_load(pAsmName, m_sProjectDllDir, &status);
	mono_assembly_name_free(pAsmName);

	if (!pAsm)
	{
		return nullptr;
	}

	IMonoLibrary* pLib = GetLibrary(pAsm);
	if (!pLib)
	{
		pLib = new CMonoLibrary(pAsm);
		m_loadedLibraries.push_back(pLib);
	}

	return pLib;
}

IMonoLibrary* CMonoRuntime::GetLibrary(MonoAssembly* pAssembly)
{
	for (Libraries::iterator it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		if (((CMonoLibrary*)*it)->GetAssembly() == pAssembly)
		{
			return *it;
		}
	}

	return nullptr;
}

bool CMonoRuntime::LaunchPluginDomain()
{
	if (m_pDomainPlugins)
	{
		StopPluginDomain();
	}

	CRY_ASSERT(m_pDomainPlugins == nullptr);

	m_pDomainPlugins = mono_domain_create_appdomain("CryEngine.Plugins", nullptr);
	mono_domain_set(m_pDomainPlugins, 1);

#ifndef _RELEASE
	mono_debug_domain_create(m_pDomainPlugins);
#endif

	if (m_pDomainPlugins)
	{
		cry_strcpy(m_sProjectDllDir, "");

		m_pLibCommon = LoadLibrary("CryEngine.Common");
		m_pLibCore = LoadLibrary("CryEngine.Core");

		return (m_pLibCommon && m_pLibCore);
	}

	return false;
}

bool CMonoRuntime::ReloadPluginDomain()
{
	bool bError = !StopPluginDomain();
	for (ExplicitLibraries::const_iterator it = m_explicitLibraries.begin(); it != m_explicitLibraries.end(); ++it)
	{
		if (!it->m_bIsExcluded)
		{
			if (!LoadBinary(it->m_binaryPath.c_str()))
			{
				bError = true;
			}
		}
	}

	return !bError;
}

bool CMonoRuntime::StopPluginDomain()
{
	if (m_pDomainPlugins)
	{
		for (Libraries::const_reverse_iterator it = m_loadedLibraries.rbegin(); it != m_loadedLibraries.rend() - 1; ++it)
		{
			(*it)->RunMethod("Shutdown");
		}

		mono_domain_set(m_pDomainGlobal, 1);
#ifndef _RELEASE
		//mono_debug_domain_unload(m_pDomainPlugins);
#endif
		mono_domain_unload(m_pDomainPlugins);

		// we remove everything BUT mscorelib, which is hosted by the global app domain
		while (m_loadedLibraries.size() > 1)
		{
			delete m_loadedLibraries.back();
			m_loadedLibraries.pop_back();
		}

		CRY_ASSERT(m_loadedLibraries.size() == 1);

		m_pLibCore = nullptr;
		m_pLibCommon = nullptr;
		m_pDomainPlugins = nullptr;
	}

	return true;
}

bool CMonoRuntime::ExcludePlugin(const char* szBinaryPath)
{
	for (ExplicitLibraries::iterator it = m_explicitLibraries.begin(); it != m_explicitLibraries.end(); ++it)
	{
		if (!stricmp(it->m_binaryPath.c_str(), szBinaryPath))
		{
			it->m_bIsExcluded = true;
			return true;
		}
	}

	return false;
}

const char* CMonoRuntime::TryGetPlugin(MonoDomain* pDomain, const char* szAssembly)
{
	for (Libraries::const_iterator it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		CMonoLibrary* pLib = static_cast<CMonoLibrary*>(*it);
		if (!pLib)
		{
			continue;
		}

		MonoImage* pImage = mono_assembly_get_image(pLib->GetAssembly());
		if (!pImage)
		{
			continue;
		}

		MonoMethodDesc* pMethodDesc = mono_method_desc_new("CryEngine.ReflectionHelper:FindPluginInstance()", true);
		MonoMethod* pMethod = mono_method_desc_search_in_image(pMethodDesc, pImage);
		if (!pMethod)
		{
			continue;
		}

		void* args[1];
		args[0] = mono_string_new(pDomain, szAssembly);

		MonoObject* pReturn = mono_runtime_invoke(pMethod, nullptr, args, nullptr);
		if (!pReturn)
		{
			continue;
		}

		MonoString* pStr = mono_object_to_string(pReturn, nullptr);
		if (!pStr)
		{
			continue;
		}

		return mono_string_to_utf8(pStr);
	}

	return nullptr;
}
