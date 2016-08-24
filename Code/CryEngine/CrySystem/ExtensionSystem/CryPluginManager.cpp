// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CryPluginManager.h"
#include <CryExtension/ICryFactory.h>
#include <CryExtension/ICryFactoryRegistry.h>

#include <CryMono/IMonoRuntime.h>

CCryPluginManager* CCryPluginManager::m_pThis = 0;

struct SPluginDescriptor
{
	string                         m_pluginName;
	string                         m_pluginClassName;
	string                         m_pluginBinaryPath;
	string                         m_pluginAssetDirectory;

	ICryPluginManager::EPluginType m_pluginType;
};

struct SPluginContainer
{
	SPluginContainer(const SPluginDescriptor& descriptor, const ICryPluginPtr& plugin)
		: m_pluginDescriptor(descriptor)
		, m_pPlugin_CPP(plugin)
		, m_pPlugin_CS(nullptr)
	{}

	SPluginContainer(const SPluginDescriptor& descriptor)
		: m_pluginDescriptor(descriptor)
		, m_pPlugin_CPP(nullptr)
		, m_pPlugin_CS(nullptr)
	{}

	bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
	{
		bool bSuccess = false;
		if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			ICryPlugin* pPlugin_CPP = GetPluginPtr_CPP();
			if (pPlugin_CPP)
			{
				pPlugin_CPP->SetBinaryDirectory(PathUtil::GetPathWithoutFilename(m_pluginDescriptor.m_pluginBinaryPath));
				pPlugin_CPP->SetAssetDirectory(m_pluginDescriptor.m_pluginAssetDirectory);

				bSuccess = pPlugin_CPP->Initialize(env, initParams);
				if (bSuccess)
				{
					// TODO: #CryPlugins: Maybe we should return true by default (only chance to fail is a corrupted flownode register method then)
					pPlugin_CPP->RegisterFlowNodes();
				}
			}
		}
		else if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
		{
			m_pPlugin_CS = gEnv->pMonoRuntime->GetPlugin(m_pluginDescriptor.m_pluginBinaryPath);

			if (m_pPlugin_CS)
			{
				m_pPlugin_CS->SetBinaryDirectory(PathUtil::GetPathWithoutFilename(m_pluginDescriptor.m_pluginBinaryPath));
				m_pPlugin_CS->SetAssetDirectory(m_pluginDescriptor.m_pluginAssetDirectory);

				// TODO: #CryPlugins: implement flownode support for mono plugins
				bSuccess = true;
			}
		}

		return bSuccess;
	}

	void OnGameStart() const
	{
		if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			if (m_pPlugin_CPP)
			{
				m_pPlugin_CPP->OnGameStart();
			}
		}
		else if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
		{
			if (m_pPlugin_CS)
			{
				m_pPlugin_CS->Call_OnGameStart();
			}
		}
	}

	void OnGameStop() const
	{
		if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			if (m_pPlugin_CPP)
			{
				m_pPlugin_CPP->OnGameStop();
			}
		}
		else if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
		{
			if (m_pPlugin_CS)
			{
				m_pPlugin_CS->Call_OnGameStop();
			}
		}
	}

	bool Shutdown()
	{
		if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			GetPluginPtr_CPP()->UnregisterFlowNodes();
			m_pPlugin_CPP.reset();

			return GetISystem()->UnloadEngineModule(m_pluginDescriptor.m_pluginBinaryPath.c_str(), m_pluginDescriptor.m_pluginClassName.c_str());
		}
		else if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
		{
			//GetPluginPtr()->UnregisterFlowNodes();
			m_pPlugin_CS = nullptr;

			return gEnv->pMonoRuntime->UnloadBinary(m_pluginDescriptor.m_pluginBinaryPath.c_str());
		}

		return false;
	}

	ICryFactory* GetFactoryPtr() const
	{
		if (m_pPlugin_CPP.get() != nullptr)
		{
			return m_pPlugin_CPP->GetFactory();
		}

		return nullptr;
	}

	ICryPlugin* GetPluginPtr_CPP() const
	{
		if (m_pPlugin_CPP)
		{
			return m_pPlugin_CPP.get();
		}

		return nullptr;
	}

	ICryEngineBasePlugin* GetPluginPtr_CS() const
	{
		return m_pPlugin_CS;
	}

	const SPluginDescriptor& GetPluginDescriptor() const
	{
		return m_pluginDescriptor;
	}

	void Update(int updateFlags, int nPauseMode)
	{
		// TODO: #CryPlugins: discuss update call source for c++ plugins
		if (m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			if (GetPluginPtr_CPP())
			{
				GetPluginPtr_CPP()->Update(updateFlags, nPauseMode);
			}
		}
	}

	friend bool operator==(const SPluginContainer& left, const SPluginContainer& right)
	{
		if (left.m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CPP)
		{
			return (left.GetPluginPtr_CPP() == right.GetPluginPtr_CPP());
		}
		else if (left.m_pluginDescriptor.m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
		{
			return (left.m_pPlugin_CS == right.m_pPlugin_CS);
		}

		return false;
	}

	SPluginDescriptor     m_pluginDescriptor;
	ICryPluginPtr         m_pPlugin_CPP;
	ICryEngineBasePlugin* m_pPlugin_CS;
};

void CCryPluginManager::ReloadPluginCmd(IConsoleCmdArgs* pArgs)
{
	m_pThis->UnloadAllPlugins();
	m_pThis->LoadExtensionFile("cryplugin.csv");
	m_pThis->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT_DONE, 0, 0);
}

CCryPluginManager::CCryPluginManager(const SSystemInitParams& initParams)
	: m_systemInitParams(initParams)
	, m_pluginListeners(1)
{
	m_pThis = this;
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

CCryPluginManager::~CCryPluginManager()
{
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);

	m_pluginListeners.Clear();

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
	case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
	case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
		for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
		{
			it->OnGameStart();
		}
		break;

	case ESYSTEM_EVENT_LEVEL_UNLOAD:
	case ESYSTEM_EVENT_GAME_MODE_SWITCH_END:
		for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
		{
			it->OnGameStop();
		}
		break;
	}
}

bool CCryPluginManager::Initialize()
{
#ifndef _RELEASE
	REGISTER_COMMAND("sys_reload_plugin", ReloadPluginCmd, 0, "Reload a plugin - not implemented yet");
	REGISTER_INT("sys_debug_plugins", 0, VF_DEV_ONLY | VF_DUMPTODISK, "Show PluginManager debug info");
#endif

	return LoadExtensionFile("cryplugin.csv");
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
	while (bResult && pTokenStart != pBufferEnd)
	{
		const char* pNewline = Parser_StrChr(pTokenStart, pBufferEnd, '\n');
		const char* pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		SPluginDescriptor desc;
		desc.m_pluginType = ICryPluginManager::EPluginType::EPluginType_CPP;
		if (Parser_StrEquals(pTokenStart, pSemicolon, "C#"))
			desc.m_pluginType = ICryPluginManager::EPluginType::EPluginType_CS;

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');
		desc.m_pluginName.assign(pTokenStart, pSemicolon - pTokenStart);
		desc.m_pluginName.Trim();

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');
		desc.m_pluginClassName.assign(pTokenStart, pSemicolon - pTokenStart);
		desc.m_pluginClassName.Trim();

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');
		desc.m_pluginBinaryPath.assign(pTokenStart, pSemicolon - pTokenStart);
		desc.m_pluginBinaryPath.Trim();

		pTokenStart = Parser_NextChar(pSemicolon, pNewline);
		pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');
		desc.m_pluginAssetDirectory.assign(pTokenStart, pSemicolon - pTokenStart);
		desc.m_pluginAssetDirectory.Trim();

		pTokenStart = Parser_NextChar(pNewline, pBufferEnd);
		bResult = LoadPlugin(desc);
	}

	delete[](pBuffer);
	return bResult;
}

bool CCryPluginManager::LoadPlugin(const SPluginDescriptor& pluginDescriptor)
{
	if (FindPluginContainer(pluginDescriptor.m_pluginName))
	{
		// Plugin is already loaded, nothing else to do
		CryLogAlways("Plugin %s has already been loaded - skipping", pluginDescriptor.m_pluginName.c_str());
		return true;
	}

	bool bSuccess = false;
	switch (pluginDescriptor.m_pluginType)
	{
	case EPluginType::EPluginType_CPP:
		{
			GetISystem()->InitializeEngineModule(pluginDescriptor.m_pluginBinaryPath, pluginDescriptor.m_pluginClassName, false);

			ICryFactoryRegistry* pFactoryRegistry = GetISystem()->GetCryFactoryRegistry();
			ICryFactory* pFactory = pFactoryRegistry->GetFactory(pluginDescriptor.m_pluginClassName);
			if (pFactory)
			{
				ICryPluginPtr pNewClassInstance = cryinterface_cast<ICryPlugin>(pFactory->CreateClassInstance());
				if (pNewClassInstance)
				{
					SPluginContainer newPlugin(pluginDescriptor, pNewClassInstance);
					bSuccess = newPlugin.Initialize(*gEnv, m_systemInitParams);
					m_pluginContainer.push_back(newPlugin);
				}
			}
			break;
		}

	case EPluginType::EPluginType_CS:
		{
			if (gEnv->pMonoRuntime)
			{
				if (gEnv->pMonoRuntime->LoadBinary(pluginDescriptor.m_pluginBinaryPath.c_str()))
				{
					SPluginContainer newPlugin(pluginDescriptor);
					bSuccess = newPlugin.Initialize(*gEnv, m_systemInitParams);
					m_pluginContainer.push_back(newPlugin);
				}
			}
			break;
		}
	}

	if (bSuccess)
	{
		// Notification to listeners, that plugin got initialized
		NotifyListeners(pluginDescriptor.m_pluginName.c_str(), IPluginEventListener::EPluginEvent::EPlugin_Initialized);
	}
	else
	{
		UnloadPlugin(pluginDescriptor);
	}

	return bSuccess;
}

bool CCryPluginManager::UnloadPlugin(const SPluginDescriptor& pluginDescriptor, bool bReloadOther)
{
	bool bSuccess = false;
	const SPluginContainer* pPluginContainer = FindPluginContainer(pluginDescriptor.m_pluginName);
	if (pPluginContainer)
	{
		TPluginList::iterator pluginIt = std::find(m_pluginContainer.begin(), m_pluginContainer.end(), *pPluginContainer);
		if (pluginIt != m_pluginContainer.end())
		{
			bSuccess = pluginIt->Shutdown();
			m_pluginContainer.erase(pluginIt);

			// TODO: #CryPlugins: work-around, this must be improved
			if (bReloadOther)
			{
				if (pluginIt->GetPluginDescriptor().m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
				{
					for (TPluginList::iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
					{
						if (it->GetPluginDescriptor().m_pluginType == ICryPluginManager::EPluginType::EPluginType_CS)
						{
							it->Initialize(*gEnv, m_systemInitParams);
						}
					}
				}
			}

			// notification to listeners, that plugin got un-initialized
			NotifyListeners(pluginDescriptor.m_pluginClassName.c_str(), IPluginEventListener::EPluginEvent::EPlugin_Unloaded);
		}
	}

	return bSuccess;
}

bool CCryPluginManager::UnloadAllPlugins()
{
	bool bError = false;
	TPluginList::const_iterator itEnd = m_pluginContainer.end();
	for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != itEnd; ++it)
	{
		if (!UnloadPlugin(it->GetPluginDescriptor()))
		{
			bError = true;
		}
	}

	return !bError;
}

void CCryPluginManager::NotifyListeners(const char* szPluginName, IPluginEventListener::EPluginEvent event)
{
	for (TPluginEventListeners::Notifier notifier(m_pluginListeners); notifier.IsValid(); notifier.Next())
	{
		if (!strcmp(notifier.Name(), szPluginName))
		{
			notifier->OnPluginEvent(szPluginName, event);
		}
	}
}

void CCryPluginManager::Update(int updateFlags, int nPauseMode)
{
	// TODO: #CryPlugins: update call should be removed at some point
	for (TPluginList::iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
	{
		it->Update(updateFlags, nPauseMode);
	}

#ifndef _RELEASE
	ICVar* pCVarPlugins = gEnv->pConsole->GetCVar("sys_debug_plugins");
	if (pCVarPlugins && pCVarPlugins->GetIVal() > 0)
	{
		const float color[4] = { 1, 1, 1, 1 };
		const float textSize = 1.5f;
		const float xMargin = 40.f;

		float yPos = gEnv->pRenderer->GetHeight() / 2.0f;
		float secondColumnOffset = 0.f;

		STextDrawContext ctx;
		ctx.SetSizeIn800x600(false);
		ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * textSize, UIDRAW_TEXTSIZEFACTOR * textSize));

		// TODO: #CryPlugins: print out proper plugin statistics
		IFFont* defaultFont = gEnv->pCryFont->GetFont("default");
		if (defaultFont)
		{
			for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
			{
				gEnv->pRenderer->Draw2dLabel(xMargin + secondColumnOffset, yPos, 1.5f, color, false, "$3%s '%s'", it->GetPluginDescriptor().m_pluginClassName.c_str(), it->GetPluginDescriptor().m_pluginBinaryPath.c_str());
				yPos -= 15.f;
			}
		}
	}
#endif
}

const SPluginContainer* CCryPluginManager::FindPluginContainer(const string& pluginName) const
{
	for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
	{
		if (it->GetPluginDescriptor().m_pluginName.find(pluginName) != string::npos)
		{
			return &(*it);
		}
	}

	return nullptr;
}

// TODO: #CryPlugins: only works for c++ plugins right now (is it needed for mono?)
ICryPluginPtr CCryPluginManager::QueryPluginById(const CryClassID& classID) const
{
	for (TPluginList::const_iterator it = m_pluginContainer.begin(); it != m_pluginContainer.end(); ++it)
	{
		ICryFactory* pFactory = it->GetFactoryPtr();
		if (pFactory)
		{
			if (pFactory->GetClassID() == classID || pFactory->ClassSupports(classID))
			{
				return it->m_pPlugin_CPP;
			}
		}
	}

	return nullptr;
}
