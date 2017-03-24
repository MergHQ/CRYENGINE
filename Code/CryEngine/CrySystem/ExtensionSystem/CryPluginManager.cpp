// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryPluginManager.h"

#include "System.h"

#include <CryExtension/ICryFactory.h>
#include <CryExtension/ICryFactoryRegistry.h>
#include <CryExtension/CryCreateClassInstance.h>

#include <CryMono/IMonoRuntime.h>

#include <CryCore/Platform/CryLibrary.h>
CCryPluginManager* CCryPluginManager::s_pThis = 0;

// Descriptor for the binary file of a plugin.
// This is separate since a plugin does not necessarily have to come from a binary, for example if static linking is used.
struct SNativePluginModule
{
	SNativePluginModule() {}

	SNativePluginModule(const char* path)
		: m_engineModulePath(path)
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "LoadPlugin");
		MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "%s", path);

		m_pFactory = gEnv->pSystem->LoadModuleWithFactory(path, cryiidof<ICryPlugin>());

		if (m_pFactory == nullptr)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Plugin load failed - valid ICryPlugin implementation was not found in plugin %s!", path);

			MarkUnloaded();
			return;
		}
	}

	SNativePluginModule(SNativePluginModule& other)
	{
		m_engineModulePath = other.m_engineModulePath;
		
		other.MarkUnloaded();
	}

	SNativePluginModule(SNativePluginModule&& other)
	{
		m_engineModulePath = other.m_engineModulePath;
		
		other.MarkUnloaded();
	}

	SNativePluginModule& operator=(const SNativePluginModule&& other)
	{
		m_engineModulePath = other.m_engineModulePath;
		
		return *this;
	}

	~SNativePluginModule()
	{
		Shutdown();
	}

	bool Shutdown()
	{
		bool bSuccess = false;
		if (m_engineModulePath.size() > 0)
		{
			bSuccess = GetISystem()->UnloadEngineModule(m_engineModulePath);

			// Prevent Shutdown twice
			MarkUnloaded();
		}

		return bSuccess;
	}

	void MarkUnloaded()
	{
		m_engineModulePath.clear();
	}

	bool IsLoaded()
	{
		return m_engineModulePath.size() > 0;
	}

	ICryFactory* GetFactory() const { return m_pFactory; }

protected:
	string m_engineModulePath;
	ICryFactory* m_pFactory;
};

struct SPluginContainer
{
	// Constructor for native plug-ins
	SPluginContainer(const std::shared_ptr<ICryPlugin>& plugin, SNativePluginModule&& module)
		: m_pPlugin(plugin)
		, m_module(module)
		, m_pluginClassId(plugin->GetFactory()->GetClassID())
	{
	}

	// Constructor for managed (Mono) plug-ins, or statically linked ones
	SPluginContainer(const std::shared_ptr<ICryPlugin>& plugin)
		: m_pPlugin(plugin) {}

	bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
	{
		bool bSuccess = false;

		if (m_pPlugin)
		{
			m_pPlugin->SetUpdateFlags(IPluginUpdateListener::EUpdateType_NoUpdate);
			bSuccess = m_pPlugin->Initialize(env, initParams);
		}

		return bSuccess;
	}

	bool Shutdown()
	{
		m_pPlugin->UnregisterFlowNodes();
		CRY_ASSERT_MESSAGE(m_pPlugin.use_count() == 2, m_pPlugin->GetName()); // Referenced here and in the factory
		m_pPlugin.reset();

		return m_module.Shutdown();
	}

	ICryPlugin* GetPluginPtr() const
	{
		return m_pPlugin ? m_pPlugin.get() : nullptr;
	}

	friend bool operator==(const SPluginContainer& left, const SPluginContainer& right)
	{
		return (left.GetPluginPtr() == right.GetPluginPtr());
	}

	CryClassID                     m_pluginClassId;
	string                         m_pluginAssetDirectory;

	ICryPluginManager::EPluginType m_pluginType;

	std::shared_ptr<ICryPlugin>    m_pPlugin;

	SNativePluginModule            m_module;
};

void CCryPluginManager::ReloadPluginCmd(IConsoleCmdArgs* pArgs)
{
	s_pThis->UnloadAllPlugins();
	s_pThis->LoadExtensionFile("cryplugin.csv");
	s_pThis->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT_DONE, 0, 0);
}

CCryPluginManager::CCryPluginManager(const SSystemInitParams& initParams)
	: m_systemInitParams(initParams)
{
	s_pThis = this;

	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this, "CCryPluginManager");
}

CCryPluginManager::~CCryPluginManager()
{
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);

	UnloadAllPlugins();

	CRY_ASSERT(m_pluginContainer.empty());

	if (gEnv->pConsole)
	{
		gEnv->pConsole->RemoveCommand("sys_reload_plugin");
		gEnv->pConsole->UnregisterVariable("sys_debug_plugin", true);
	}
}

void CCryPluginManager::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_REGISTER_FLOWNODES:
		{
			for (auto& it : m_pluginContainer)
			{
				it.GetPluginPtr()->RegisterFlowNodes();
			}
		}
		break;
	}
}

void CCryPluginManager::Initialize()
{
#ifndef _RELEASE
	REGISTER_COMMAND("sys_reload_plugin", ReloadPluginCmd, 0, "Reload a plugin - not implemented yet");
#endif

	// Start with loading the default engine plug-ins
	LoadPluginFromDisk(EPluginType::EPluginType_CPP, "CryDefaultEntities");
	//Schematyc + Schematyc Standard Enviroment
	LoadPluginFromDisk(EPluginType::EPluginType_CPP, "CrySchematycCore");
	LoadPluginFromDisk(EPluginType::EPluginType_CPP, "CrySchematycSTDEnv");
	//optional plugin, but we load it for now by default

	LoadPluginFromDisk(EPluginType::EPluginType_CPP, "CrySensorSystem");

	LoadExtensionFile("cryplugin.csv");
}

//--- UTF8 parse helper routines

static const char* Parser_NextChar(const char* pStart, const char* pEnd)
{
	CRY_ASSERT(pStart != nullptr && pEnd != nullptr);

	if (pStart < pEnd)
	{
		CRY_ASSERT(0 <= *pStart && *pStart <= SCHAR_MAX);
		pStart++;
	}

	CRY_ASSERT(pStart <= pEnd);
	return pStart;
}

static const char* Parser_StrChr(const char* pStart, const char* pEnd, int c)
{
	CRY_ASSERT(pStart != nullptr && pEnd != nullptr);
	CRY_ASSERT(pStart <= pEnd);
	CRY_ASSERT(0 <= c && c <= SCHAR_MAX);
	const char* it = (const char*)memchr(pStart, c, pEnd - pStart);
	return (it != nullptr) ? it : pEnd;
}

static bool Parser_StrEquals(const char* pStart, const char* pEnd, const char* szKey)
{
	size_t klen = strlen(szKey);
	return (klen == pEnd - pStart) && memcmp(pStart, szKey, klen) == 0;
}

//---

bool CCryPluginManager::LoadExtensionFile(const char* szFilename)
{
	CryLogAlways("Loading extension file %s", szFilename);

	CRY_ASSERT(szFilename != nullptr);
	bool bResult = true;

	ICryPak* pCryPak = gEnv->pCryPak;
	FILE* pFile = pCryPak->FOpen(szFilename, "rb", ICryPak::FLAGS_PATH_REAL);
	if (pFile == nullptr)
		return bResult;

	size_t iFileSize = pCryPak->FGetSize(pFile);
	char* pBuffer = new char[iFileSize];
	pCryPak->FReadRawAll(pBuffer, iFileSize, pFile);
	pCryPak->FClose(pFile);

	const char* pTokenStart = pBuffer;
	const char* pBufferEnd = pBuffer + iFileSize;
	while (pTokenStart != pBufferEnd)
	{
		const char* pNewline = Parser_StrChr(pTokenStart, pBufferEnd, '\n');
		const char* pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		auto pluginType = ICryPluginManager::EPluginType::EPluginType_CPP;
		if (Parser_StrEquals(pTokenStart, pSemicolon, "C#"))
			pluginType = ICryPluginManager::EPluginType::EPluginType_CS;

		// Parsing of plugin name
		// Not actually used anymore, but no need to break existing setup seeing as we're going to move away from csv soon anyway - Filip
		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		string pluginClassName;
		pluginClassName.assign(pTokenStart, pSemicolon - pTokenStart);
		pluginClassName.Trim();

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		string pluginBinaryPath;
		pluginBinaryPath.assign(pTokenStart, pSemicolon - pTokenStart);
		pluginBinaryPath.Trim();

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		string pluginAssetDirectory;
		pluginAssetDirectory.assign(pTokenStart, pSemicolon - pTokenStart);
		pluginAssetDirectory.Trim();
	#pragma message("TODO: Use plugin asset directory")

		pTokenStart = Parser_NextChar(pNewline, pBufferEnd);
		if (!LoadPluginFromDisk(pluginType, pluginBinaryPath))
		{
			bResult = false;
		}
	}

	delete[](pBuffer);
	return bResult;
}

bool CCryPluginManager::LoadPluginFromDisk(EPluginType type, const char* path)
{
	CryLogAlways("Loading plugin %s", path);

	std::shared_ptr<ICryPlugin> pPlugin;

	switch (type)
	{
	case EPluginType::EPluginType_CPP:
		{
			// Load the module, note that this calls ISystem::InitializeEngineModule
			// Automatically unloads in destructor
			SNativePluginModule module(path);

			if (auto pFactory = module.GetFactory())
			{
				ICryUnknownPtr pUnk = pFactory->CreateClassInstance();
				pPlugin = cryinterface_cast<ICryPlugin>(pUnk);
				if (!pPlugin)
				{
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Plugin load failed - Could not create an instance of %s in plugin %s!", pFactory->GetName(), path);
					return false;
				}

				m_pluginContainer.emplace_back(pPlugin, std::move(module));
				module.MarkUnloaded();
			}
			else
			{
				return false;
			}

			break;
		}

	case EPluginType::EPluginType_CS:
		{
			if (gEnv->pMonoRuntime == nullptr)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Tried to load Mono plugin %s without having loaded the CryMono module!", path);

				return false;
			}

			pPlugin = gEnv->pMonoRuntime->LoadBinary(path);
			if (!pPlugin)
			{
				CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Plugin load failed - Could not load Mono binary %s", path);

				return false;
			}

			m_pluginContainer.emplace_back(pPlugin);
			break;
		}
	}

	auto &containedPlugin = m_pluginContainer.back();

	if (!containedPlugin.Initialize(*gEnv, m_systemInitParams))
	{
		CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Plugin load failed - Failed to initialize plugin %s!", path);

		containedPlugin.Shutdown();
		m_pluginContainer.pop_back();

		return false;
	}

	// Notification to listeners, that plugin got initialized
	NotifyEventListeners(containedPlugin.m_pluginClassId, IPluginEventListener::EPluginEvent::Initialized);
	return true;
}

bool CCryPluginManager::UnloadAllPlugins()
{
	bool bError = false;
	for (auto& it : m_pluginContainer)
	{
		if (!it.Shutdown())
		{
			bError = true;
		}

		// notification to listeners, that plugin got un-initialized
		NotifyEventListeners(it.m_pluginClassId, IPluginEventListener::EPluginEvent::Unloaded);
	}

	m_pluginContainer.clear();

	return !bError;
}

void CCryPluginManager::NotifyEventListeners(const CryClassID& classID, IPluginEventListener::EPluginEvent event)
{
	for (const auto& it : m_pluginListenerMap)
	{
		const auto& classIDs = it.second;
		if (std::find(classIDs.cbegin(), classIDs.cend(), classID) != classIDs.cend())
		{
			it.first->OnPluginEvent(classID, event);
		}
	}
}

void CCryPluginManager::Update(IPluginUpdateListener::EPluginUpdateType updateFlags)
{
	for (const auto& it : m_pluginContainer)
	{
		auto pPlugin = it.GetPluginPtr();
		CRY_ASSERT(pPlugin != nullptr);
		if ((pPlugin->GetUpdateFlags() & updateFlags) != 0)
		{
			pPlugin->OnPluginUpdate(updateFlags);
		}
	}
}

std::shared_ptr<ICryPlugin> CCryPluginManager::QueryPluginById(const CryClassID& classID) const
{
	for (const auto& it : m_pluginContainer)
	{
		ICryFactory* pFactory = it.GetPluginPtr()->GetFactory();
		if (pFactory)
		{
			if (pFactory->GetClassID() == classID || pFactory->ClassSupports(classID))
			{
				return it.m_pPlugin;
			}
		}
	}

	return nullptr;
}


std::shared_ptr<ICryPlugin> CCryPluginManager::AcquirePluginById(const CryClassID& classID)
{
	std::shared_ptr<ICryPlugin> pPlugin;
	CryCreateClassInstance(classID, pPlugin);

	if (pPlugin == nullptr)
	{
		return nullptr;
	}

	m_pluginContainer.emplace_back(pPlugin);
	if (!m_pluginContainer.back().Initialize(*gEnv, m_systemInitParams))
	{
		m_pluginContainer.back().Shutdown();
		m_pluginContainer.pop_back();

		return nullptr;
	}

	return pPlugin;
}
