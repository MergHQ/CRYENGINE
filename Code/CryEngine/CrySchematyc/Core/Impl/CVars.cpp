// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CVars.h"

namespace Schematyc
{
ICVar* CVars::sc_FileFormat = nullptr;
ICVar* CVars::sc_RootFolder = nullptr;
int CVars::sc_IgnorePAKFiles = 0;
int CVars::sc_IgnoreUnderscoredFolders = 1;
int CVars::sc_EnableScriptPartitioning = 0;

int CVars::sc_LogToFile = 0;
ICVar* CVars::sc_LogFileStreams = nullptr;
ICVar* CVars::sc_LogFileMessageTypes = nullptr;
int CVars::sc_DisplayCriticalErrors = 0;

int CVars::sc_RunUnitTests = 0;
ICVar* CVars::sc_ExperimentalFeatures = nullptr;

void CVars::Register()
{
	sc_FileFormat = REGISTER_STRING("sc_FileFormat", "xml", VF_READONLY, "Schematyc - Format of Schematyc files");
	sc_RootFolder = REGISTER_STRING("sc_RootFolder", "libs/schematyc", VF_READONLY, "Schematyc - Root folder for files");
	REGISTER_CVAR(sc_IgnorePAKFiles, sc_IgnorePAKFiles, VF_READONLY, "Schematyc - Ignore PAK files");
	REGISTER_CVAR(sc_IgnoreUnderscoredFolders, sc_IgnoreUnderscoredFolders, VF_READONLY, "Schematyc - Ignore folders whose names begin with an underscore");
	REGISTER_CVAR(sc_EnableScriptPartitioning, sc_EnableScriptPartitioning, VF_READONLY, "Schematyc - Enable partitioning of scripts");

	REGISTER_CVAR(sc_LogToFile, sc_LogToFile, VF_NULL, "Schematyc - Enable/disable sending of log messages to file");
	sc_LogFileStreams = REGISTER_STRING("sc_LogFileStreams", "Default System Compiler Game", VF_NULL, "Schematyc - Set which streams should be sent to log file");
	sc_LogFileMessageTypes = REGISTER_STRING("sc_LogFileMessageTypes", "Warning Error CriticalError", VF_NULL, "Schematyc - Set which message types should be sent to log file");
	REGISTER_CVAR(sc_DisplayCriticalErrors, sc_DisplayCriticalErrors, VF_NULL, "Schematyc - Display critical error message alerts");

	REGISTER_CVAR(sc_RunUnitTests, sc_RunUnitTests, VF_READONLY, "Schematyc - Enable/disable unit tests on startup");
	sc_ExperimentalFeatures = REGISTER_STRING("sc_ExperimentalFeatures", "", VF_NULL, "Schematyc - Enable one or more experimental features");
}

void CVars::Unregister()
{
	gEnv->pConsole->UnregisterVariable("sc_FileFormat");
	gEnv->pConsole->UnregisterVariable("sc_RootFolder");
	gEnv->pConsole->UnregisterVariable("sc_IgnorePAKFiles");
	gEnv->pConsole->UnregisterVariable("sc_IgnoreUnderscoredFolders");
	gEnv->pConsole->UnregisterVariable("sc_EnableScriptPartitioning");
	gEnv->pConsole->UnregisterVariable("sc_LogToFile");
	gEnv->pConsole->UnregisterVariable("sc_LogFileStreams");
	gEnv->pConsole->UnregisterVariable("sc_LogFileMessageTypes");
	gEnv->pConsole->UnregisterVariable("sc_DisplayCriticalErrors");
	gEnv->pConsole->UnregisterVariable("sc_RunUnitTests");
	gEnv->pConsole->UnregisterVariable("sc_ExperimentalFeatures");
}
} // Schematyc
