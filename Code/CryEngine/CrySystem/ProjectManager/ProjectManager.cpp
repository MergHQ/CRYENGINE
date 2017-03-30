#include "StdAfx.h"
#include "ProjectManager.h"

#include "System.h"

#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/STL.h>
#include <CrySerialization/Enum.h>

using namespace Cry::ProjectManagerInternals;

#if CRY_PLATFORM_WINDOWS
#include <Shlwapi.h>
#endif

YASLI_ENUM_BEGIN_NESTED(ICryPluginManager, EPluginType, "PluginType")
YASLI_ENUM_VALUE_NESTED(ICryPluginManager, EPluginType::Native, "Native")
YASLI_ENUM_VALUE_NESTED(ICryPluginManager, EPluginType::Managed, "Managed")
YASLI_ENUM_END()

CProjectManager::CProjectManager()
{
	CryFindRootFolderAndSetAsCurrentWorkingDirectory();
}

const char* CProjectManager::GetCurrentProjectName()
{
	return gEnv->pConsole->GetCVar("sys_game_name")->GetString();
}

const char* CProjectManager::GetCurrentProjectDirectoryAbsolute()
{
	return m_project.rootDirectory;
}

const char* CProjectManager::GetCurrentAssetDirectoryRelative()
{
	return gEnv->pCryPak->GetGameFolder();
}

const char* CProjectManager::GetCurrentAssetDirectoryAbsolute()
{ 
	return m_project.assetDirectoryFullPath;
}

void CProjectManager::StoreConsoleVariable(const char* szCVarName, const char* szValue)
{
	for (auto it = m_project.consoleVariables.begin(); it != m_project.consoleVariables.end(); ++it)
	{
		if (!stricmp(it->key, szCVarName))
		{
			it->value = szValue;

			return;
		}
	}

	m_project.consoleVariables.emplace_back(szCVarName, szValue);
}

void CProjectManager::SaveProjectChanges()
{
	gEnv->pSystem->GetArchiveHost()->SaveJsonFile(m_project.filePath, Serialization::SStruct(m_project));
}

void SProject::Serialize(Serialization::IArchive& ar)
{
	// Only save to the latest format
	if (ar.isOutput())
	{
		version = LatestProjectFileVersion;
	}

	ar(version, "version", "version");
	if (version == 0 || version == 1)
	{
		SProjectFileParser<1> parser;
		parser.Serialize(ar, *this);
	}
}

void CProjectManager::ParseProjectFile()
{
	const ICmdLineArg* arg = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "project");
	const char* szProjectFile = arg != nullptr ? arg->GetValue() : "game.cryproject";
	
#if CRY_PLATFORM_DURANGO
	if(true)
#elif CRY_PLATFORM_WINAPI
	if (PathIsRelative(szProjectFile))
#elif CRY_PLATFORM_POSIX
	if (szProjectFile[0] != '/')
#endif
	{
		char buffer[MAX_PATH];
		CryGetCurrentDirectory(MAX_PATH, buffer);

		m_project.filePath = PathUtil::Make(buffer, szProjectFile);
	}
	else
	{
		m_project.filePath = szProjectFile;
	}

	if (gEnv->pSystem->GetArchiveHost()->LoadJsonFile(Serialization::SStruct(m_project), m_project.filePath))
	{
		m_project.rootDirectory = PathUtil::RemoveSlash(PathUtil::ToUnixPath(PathUtil::GetPathWithoutFilename(m_project.filePath)));

		// Create the full path to the asset directory
		m_project.assetDirectoryFullPath = PathUtil::Make(m_project.rootDirectory, m_project.assetDirectory);

		// Set the game folder and name
		gEnv->pConsole->LoadConfigVar("sys_game_folder", m_project.assetDirectory);
		gEnv->pConsole->LoadConfigVar("sys_game_name", m_project.name);

		for (SProject::SConsoleVariable& consoleVariable : m_project.consoleVariables)
		{
			gEnv->pConsole->LoadConfigVar(consoleVariable.key, consoleVariable.value);
		}

		auto gameDllIt = m_project.legacyGameDllPaths.find("any");

#if (CRY_PLATFORM_WINDOWS && CRY_PLATFORM_64BIT)
		if (gameDllIt == m_project.legacyGameDllPaths.end())
		{
			gameDllIt = m_project.legacyGameDllPaths.find("win_x64");
		}
#elif (CRY_PLATFORM_WINDOWS && CRY_PLATFORM_32BIT)
		if (gameDllIt == m_project.legacyGameDllPaths.end())
		{
			gameDllIt = m_project.legacyGameDllPaths.find("win_x86");
		}
#endif

		if (gameDllIt != m_project.legacyGameDllPaths.end())
		{
			string legacyGameDllPath = gameDllIt->second;

			gEnv->pConsole->LoadConfigVar("sys_dll_game", legacyGameDllPath);
		}

#ifdef CRY_PLATFORM_WINDOWS
		SetDllDirectoryW(CryStringUtils::UTF8ToWStr(m_project.rootDirectory));
#endif

#ifndef CRY_PLATFORM_ORBIS
		CrySetCurrentWorkingDirectory(m_project.rootDirectory);
#endif

		// Update the project file if the loaded version was outdated
		if (m_project.version != LatestProjectFileVersion)
		{
			// Add default plug-ins since they were hardcoded before version 1
			if (m_project.version == 0)
			{
				AddDefaultPlugins();
				LoadLegacyPluginCSV();
				LoadLegacyGameCfg();
			}

			m_project.version = LatestProjectFileVersion;

			SaveProjectChanges();
		}
	}
}

void CProjectManager::MigrateFromLegacyWorkflowIfNecessary()
{
	// Populate project data and save .cryproject if no project was used
	// This is done by assuming legacy game folder setup.
	if (m_project.version == 0 && strlen(GetCurrentAssetDirectoryRelative()) > 0)
	{
		m_project.version = LatestProjectFileVersion;
		m_project.type = "CRYENGINE Project";
		m_project.name = gEnv->pConsole->GetCVar("sys_game_name")->GetString();

		// TODO: Detect latest using CrySelect?
		m_project.engineVersionId = "engine-5.4";

		char buffer[MAX_PATH];
		CryGetCurrentDirectory(MAX_PATH, buffer);
		
		m_project.rootDirectory = PathUtil::RemoveSlash(PathUtil::ToUnixPath(buffer));
		m_project.assetDirectory = GetCurrentAssetDirectoryRelative();

		// Create the full path to the asset directory
		m_project.assetDirectoryFullPath = PathUtil::Make(m_project.rootDirectory, m_project.assetDirectory);

		// Make sure we include default plug-ins
		AddDefaultPlugins();
		LoadLegacyPluginCSV();
		LoadLegacyGameCfg();

		const char* legacyDllName = gEnv->pConsole->GetCVar("sys_dll_game")->GetString();
		if (strlen(legacyDllName) > 0)
		{
			m_project.legacyGameDllPaths["any"] = PathUtil::RemoveExtension(legacyDllName);
		}

		string sProjectFile = PathUtil::Make(m_project.rootDirectory, "Game.cryproject");
		gEnv->pSystem->GetArchiveHost()->SaveJsonFile(sProjectFile, Serialization::SStruct(m_project));
	}
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

void CProjectManager::LoadLegacyPluginCSV()
{
	FILE* pFile = gEnv->pCryPak->FOpen("cryplugin.csv", "rb", ICryPak::FLAGS_PATH_REAL);
	if (pFile == nullptr)
		return;

	std::vector<char> buffer;
	buffer.resize(gEnv->pCryPak->FGetSize(pFile));

	gEnv->pCryPak->FReadRawAll(buffer.data(), buffer.size(), pFile);
	gEnv->pCryPak->FClose(pFile);

	const char* pTokenStart = buffer.data();
	const char* pBufferEnd = buffer.data() + buffer.size();
	while (pTokenStart != pBufferEnd)
	{
		const char* pNewline = Parser_StrChr(pTokenStart, pBufferEnd, '\n');
		const char* pSemicolon = Parser_StrChr(pTokenStart, pNewline, ';');

		ICryPluginManager::EPluginType pluginType = ICryPluginManager::EPluginType::Native;
		if (Parser_StrEquals(pTokenStart, pSemicolon, "C#"))
			pluginType = ICryPluginManager::EPluginType::Managed;

		// Parsing of plugin name
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

		pTokenStart = Parser_NextChar(pNewline, pBufferEnd);

		AddPlugin(pluginType, pluginBinaryPath);
	}
}

void CProjectManager::LoadLegacyGameCfg()
{
	string cfgPath = PathUtil::Make(m_project.assetDirectoryFullPath, "game.cfg");
	gEnv->pSystem->LoadConfiguration(cfgPath, this);
}

void CProjectManager::OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
{
	// Make sure we set the value in the engine as well
	gEnv->pConsole->LoadConfigVar(szKey, szValue);

	// Store in our cryproject json, unless the CVar is one that we have in other properties
	if (strcmp(szKey, "sys_dll_game") != 0 && strcmp(szKey, "sys_game_name") != 0)
	{
		StoreConsoleVariable(szKey, szValue);
	}
}

void CProjectManager::AddDefaultPlugins()
{
	AddPlugin(ICryPluginManager::EPluginType::Native, "CryDefaultEntities");
	AddPlugin(ICryPluginManager::EPluginType::Native, "CrySchematycCore");
	AddPlugin(ICryPluginManager::EPluginType::Native, "CrySchematycSTDEnv");
	AddPlugin(ICryPluginManager::EPluginType::Native, "CrySensorSystem");
	AddPlugin(ICryPluginManager::EPluginType::Native, "CryPerceptionSystem");
}

void CProjectManager::AddPlugin(ICryPluginManager::EPluginType type, const char* szFileName)
{
	// Make sure duplicates aren't added
	for(const SPluginDefinition& pluginDefinition : m_project.plugins)
	{
		if (!stricmp(pluginDefinition.path, szFileName))
		{
			return;
		}
	}

	m_project.plugins.emplace_back(type, szFileName);
}