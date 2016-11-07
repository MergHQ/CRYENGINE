#include "StdAfx.h"
#include "ProjectManager.h"

#include "System.h"

#include <jsmn.h>
#include <jsmnutil.h>

static string file_get_contents(const string& sProjectFile)
{
#if CRY_PLATFORM_WINDOWS
	FILE* file = _wfopen(CryStringUtils::UTF8ToWStr(sProjectFile), L"rb");
#else
	FILE* file = fopen(sProjectFile.c_str(), "rb");
#endif

	string buffer;
	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);

		buffer.resize(size);

		fseek(file, 0, SEEK_SET);
		fread((void*)&buffer[0], size, 1, file);
		fclose(file);
	}

	return buffer;
}

static std::vector<jsmntok_t> json_decode(const string& buffer)
{
	std::vector<jsmntok_t> tokens;
	tokens.resize(64);

	jsmn_parser parser;
	jsmn_init(&parser);
	int ntokens = jsmn_parse(&parser, buffer.data(), buffer.size(), tokens.data(), tokens.size());
	while (ntokens == JSMN_ERROR_NOMEM)
	{
		tokens.resize(tokens.size() * 2);
		ntokens = jsmn_parse(&parser, buffer.data(), buffer.size(), tokens.data(), tokens.size());
	}

	if (0 <= ntokens)
		tokens.resize(ntokens);
	else
		tokens.clear();

	return tokens;
}

CProjectManager::CProjectManager()
{
	LoadConfiguration();
}

const char* CProjectManager::GetCurrentProjectName()
{
	return gEnv->pConsole->GetCVar("sys_game_name")->GetString();
}

const char* CProjectManager::GetCurrentProjectDirectoryAbsolute()
{
	return m_currentProjectDirectory;
}

const char* CProjectManager::GetCurrentAssetDirectoryRelative()
{
	return gEnv->pCryPak->GetGameFolder();
}

const char* CProjectManager::GetCurrentAssetDirectoryAbsolute()
{ 
	if (!m_bEnabled && m_currentAssetDirectory.size() == 0)
	{
		m_currentAssetDirectory = PathUtil::ToUnixPath(PathUtil::Make(m_currentProjectDirectory, GetCurrentAssetDirectoryRelative()));

	}

	return m_currentAssetDirectory;
}

const char* CProjectManager::GetCurrentBinaryDirectoryAbsolute()
{
	return m_currentBinaryDirectory;
}

void CProjectManager::LoadConfiguration()
{
	CryFindRootFolderAndSetAsCurrentWorkingDirectory();

	const ICmdLineArg* arg = gEnv->pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "project");
	if (arg == nullptr)
	{
		// No project setup, default to legacy workflow
		m_bEnabled = false;

		char buffer[MAX_PATH];
		CryGetCurrentDirectory(MAX_PATH, buffer);

		m_currentProjectDirectory = PathUtil::ToUnixPath(buffer);

		CryGetExecutableFolder(MAX_PATH, buffer);
		m_currentBinaryDirectory = PathUtil::ToUnixPath(buffer);

		return;
	}

	m_bEnabled = true;

	const char* sProjectFile = arg->GetValue();

	string js = file_get_contents(sProjectFile);

	// Set the current directory
	m_currentProjectDirectory = PathUtil::ToUnixPath(PathUtil::GetPathWithoutFilename(sProjectFile));

#ifdef CRY_PLATFORM_WINDOWS
	SetDllDirectoryW(CryStringUtils::UTF8ToWStr(m_currentProjectDirectory));
#endif

#ifndef CRY_PLATFORM_ORBIS
	CrySetCurrentWorkingDirectory(m_currentProjectDirectory);
#endif

	std::vector<jsmntok_t> tokens = json_decode(js);

	const jsmntok_t* assets = jsmnutil_xpath(js.data(), tokens.data(), "content", "assets", "0", 0);
	if (assets != nullptr && assets->type == JSMN_STRING)
	{
		string sSysGameFolder(js.data() + assets->start, assets->end - assets->start);
		m_currentAssetDirectory = PathUtil::ToUnixPath(PathUtil::Make(m_currentProjectDirectory, sSysGameFolder));

		gEnv->pConsole->LoadConfigVar("sys_game_folder", sSysGameFolder.c_str());
	}
	else
	{
		// Make sure to clear the game folder, since this project does not require one
		gEnv->pConsole->LoadConfigVar("sys_game_folder", "");
	}

#if (CRY_PLATFORM_WINDOWS && CRY_PLATFORM_64BIT)
	const jsmntok_t* shared = jsmnutil_xpath(js.data(), tokens.data(), "content", "libs", "0", "shared", "win_x64", 0);
#elif (CRY_PLATFORM_WINDOWS && CRY_PLATFORM_32BIT)
	const jsmntok_t* shared = jsmnutil_xpath(js.data(), tokens.data(), "content", "libs", "0", "shared", "win_x86", 0);
#else
	const jsmntok_t* shared = nullptr;
#endif
	if (shared && shared->type == JSMN_STRING)
	{
		string sSysDllGame(js.data() + shared->start, shared->end - shared->start);
		
		gEnv->pConsole->LoadConfigVar("sys_dll_game", sSysDllGame);
		m_currentBinaryDirectory = PathUtil::ToUnixPath(PathUtil::Make(m_currentProjectDirectory, PathUtil::GetPathWithoutFilename(sSysDllGame)));
	}
	else
	{
		// Make sure to clear the dll path, since this project does not require one
		gEnv->pConsole->LoadConfigVar("sys_dll_game", "");
	}

	const jsmntok_t* projectName = jsmnutil_xpath(js.data(), tokens.data(), "info", "name", 0);
	if (projectName != nullptr && projectName->type == JSMN_STRING)
	{
		string sGameName(js.data() + projectName->start, projectName->end - projectName->start);

		gEnv->pConsole->LoadConfigVar("sys_game_name", sGameName);
	}
}