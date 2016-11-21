// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Plugin.h"

#include <Schematyc/Compiler/ICompiler.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/SerializationUtils/SerializationEnums.inl>
#include <Schematyc/Utils/GUID.h>

const char* g_szPluginGUID = "{91A8A207-F8F0-4D5B-B8CA-613B4920581F}";
const int g_pluginVersion = 1;
const char* g_szPluginName = "Schematyc Plug-in";

Schematyc::SGUID GenerateGUID()
{
	Schematyc::SGUID guid;
#if CRY_PLATFORM_WINDOWS
	::CoCreateGuid(&guid);
#endif
	return guid;
}

CSchematycPlugin::CSchematycPlugin()
{
	// Hook up GUID generator then fix-up script files and resolve broken/deprecated dependencies.
	CryLogAlways("[SchematycEditor]: Initializing...");
	gEnv->pSchematyc->SetGUIDGenerator(Schematyc::Delegate::Make(GenerateGUID));
	CryLogAlways("[SchematycEditor]: Fixing up script files");
	gEnv->pSchematyc->GetScriptRegistry().ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorFixUp));
	CryLogAlways("[SchematycEditor]: Compiling script files");
	gEnv->pSchematyc->GetCompiler().CompileAll();
	CryLogAlways("[SchematycEditor]: Initialization complete");
}

void CSchematycPlugin::Release()
{
	delete this;
}

void        CSchematycPlugin::ShowAbout() {}

const char* CSchematycPlugin::GetPluginGUID()
{
	return g_szPluginGUID;
}

DWORD CSchematycPlugin::GetPluginVersion()
{
	return g_pluginVersion;
}

const char* CSchematycPlugin::GetPluginName()
{
	return g_szPluginName;
}

bool CSchematycPlugin::CanExitNow()
{
	return true;
}

void CSchematycPlugin::Serialize(FILE* hFile, bool bIsStoring) {}

void CSchematycPlugin::ResetContent()                          {}

bool CSchematycPlugin::CreateUIElements()
{
	return true;
}

void CSchematycPlugin::OnEditorNotify(EEditorNotifyEvent eventId) {}
