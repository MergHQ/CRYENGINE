// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ConsoleBatchFile.h"
#include <CrySystem/ConsoleRegistration.h>
#include <CrySystem/ISystem.h>
#include "XConsole.h"
#include <CryString/CryPath.h>
#include <stdio.h>
#include "System.h"
#include "SystemCFG.h"

class CConsoleBatchSink : public ILoadConfigurationEntrySink
{
public:
	CConsoleBatchSink(bool ignoreWhitelist) 
		: m_pConsole(gEnv->pConsole), m_ignoreWhitelist(ignoreWhitelist)
	{}

	virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup) override
	{
		if (m_ignoreWhitelist || (gEnv->pSystem->IsCVarWhitelisted(szKey, false)))
		{
			gEnv->pConsole->ExecuteString((string(szKey) + " " + szValue).c_str());
		}
#if defined(DEDICATED_SERVER)
		else
		{
			gEnv->pSystem->GetILog()->LogError("Failed to apply CVar/ execute command '%s' as it is not whitelisted\n", szKey);
		}
#endif   // defined(DEDICATED_SERVER)
	}

private:
	IConsole* m_pConsole;
	bool m_ignoreWhitelist;
};

void CConsoleBatchFile::Init()
{
	REGISTER_COMMAND("exec", (ConsoleCommandFunc)ExecuteFileCmdFunc, 0, "Executes a batch file of console commands");
}

void CConsoleBatchFile::ExecuteFileCmdFunc(IConsoleCmdArgs* args)
{
	if (args->GetArgCount() < 2)
		return;

	const string filename = args->GetArg(1);
	CCryFile file;
	{
		if (!CSystemConfiguration::OpenFile(filename, file, ICryPak::FOPEN_ONDISK))
		{
			CryLog("Searching console batch file (try game, config, root): \"%s\" not found!", filename.c_str());
			return;
		}
		CryLog("Found console batch file \"%s\" at %s", filename.c_str(), file.GetFilename());
	}

	// Only circumvent whitelist when the file was in an archive, since we expect archives to be signed in release mode when whitelist is used
	// Note that we still support running release mode without signed / encrypted archives, however this means that a conscious decision to sacrifice security has already been made.
	const bool ignoreWhitelist = file.IsInPak();
	CConsoleBatchSink sink(ignoreWhitelist);
	gEnv->pSystem->LoadConfiguration(filename.c_str(), &sink, ELoadConfigurationType::eLoadConfigDefault);
}
