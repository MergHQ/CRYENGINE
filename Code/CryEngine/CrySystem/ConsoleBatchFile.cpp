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
#ifdef RELEASE
	const bool ignoreWhitelist = file.IsInPak();
#else
	const bool ignoreWhitelist = true;
#endif

	std::vector<char> fileContents(file.GetLength() + 1);
	file.ReadRaw(fileContents.data(), file.GetLength());
	fileContents.back() = '\0';
	
	char* const strLast = &fileContents.back() + 1;
	char* str = fileContents.data();
	while (str < strLast)
	{
		char* szLineStart = str;
		// find the first line break
		while (str < strLast && *str != '\n' && *str != '\r')
			str++;
		// clear it to handle the line as one string
		*str = '\0';
		str++;
		// and skip any remaining line break chars
		while (str < strLast && (*str == '\n' || *str == '\r'))
			str++;

		//trim all whitespace characters at the beginning and the end of the current line
		string strLine = szLineStart;
		strLine.Trim();

		//skip empty lines
		if (strLine.empty())
			continue;

		//skip comments, comments start with ";" or "--" (any preceding whitespaces have already been trimmed)
		if (strLine[0] == ';' || strLine.find("--") == 0)
			continue;

		if (ignoreWhitelist || (gEnv->pSystem->IsCVarWhitelisted(strLine, false)))
		{
			gEnv->pConsole->ExecuteString(strLine);
		}
#if defined(DEDICATED_SERVER)
		else
		{
			gEnv->pSystem->GetILog()->LogError("Failed to apply CVar/ execute command '%s' as it is not whitelisted\n", strLine.c_str());
		}
#endif // defined(DEDICATED_SERVER)
	}
}
