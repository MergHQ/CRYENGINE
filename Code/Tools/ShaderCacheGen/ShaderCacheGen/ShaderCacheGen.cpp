// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

// Insert your headers here
#include <CryCore/Platform/platform_impl.inl>
#include <CryCore/Platform/CryWindows.h>
#include <CryCore/Platform/CryLibrary.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/SystemInitParams.h>
#include <CrySystem/IConsole.h>
#include <CryString/CryWinStringUtils.h>
#include <ShellAPI.h>

// We need shell api for Current Root Extraction.
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

struct COutputPrintSink : public IOutputPrintSink
{
	virtual void Print( const char *inszText )
	{
		printf("%s\n", inszText);
	}
};

//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	SSystemInitParams startupParams;
	startupParams.sLogFileName = "ShaderCacheGen.log";

	startupParams.bShaderCacheGen = true;
	startupParams.bDedicatedServer = false;
	startupParams.bMinimal = true;
	startupParams.bSkipFont = true;

	COutputPrintSink printSink;
	startupParams.pPrintSync = &printSink;

	// Note: lpCmdLine does not contain the filename.
	string commandLine = CryStringUtils::ANSIToUTF8(GetCommandLineA());
	cry_strcpy(startupParams.szSystemCmdLine, commandLine.c_str());

	return CryInitializeEngine(startupParams) ? EXIT_SUCCESS : EXIT_FAILURE;
}

