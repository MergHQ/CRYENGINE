// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Plugin.h"

#include <CryCore/Platform/platform_impl.inl>

#include "IResourceSelectorHost.h"

#include <Schematyc/Compiler/ICompiler.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/SerializationUtils/SerializationEnums.inl>
#include <Schematyc/Utils/GUID.h>

#include "CryLinkCommands.h"

const int g_pluginVersion = 1;
const char* g_szPluginName = "Schematyc Plugin";
const char* g_szPluginDesc = "Schematyc Editor integration";

Schematyc::SGUID GenerateGUID()
{
	Schematyc::SGUID guid;
#if CRY_PLATFORM_WINDOWS
	GUID winGuid;
	::CoCreateGuid(&winGuid);
	static_assert(sizeof(winGuid)==sizeof(guid),"GUID and CryGUID sizes should match.");
	memcpy(&guid,&winGuid,sizeof(guid));
#else
	guid = Schematyc::SGUID::Create();
#endif
	return guid;
}

REGISTER_PLUGIN(CSchematycPlugin);

CSchematycPlugin::CSchematycPlugin()
{
	RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

	Schematyc::CCryLinkCommands::GetInstance().Register(g_pEditor->GetSystem()->GetIConsole());

	// Hook up GUID generator then fix-up script files and resolve broken/deprecated dependencies.
	CryLogAlways("[SchematycEditor]: Initializing...");
	gEnv->pSchematyc->SetGUIDGenerator(SCHEMATYC_DELEGATE(&GenerateGUID));
	CryLogAlways("[SchematycEditor]: Fixing up script files");
	gEnv->pSchematyc->GetScriptRegistry().ProcessEvent(Schematyc::SScriptEvent(Schematyc::EScriptEventId::EditorFixUp));
	CryLogAlways("[SchematycEditor]: Compiling script files");
	gEnv->pSchematyc->GetCompiler().CompileAll();
	CryLogAlways("[SchematycEditor]: Initialization complete");
}

int32 CSchematycPlugin::GetPluginVersion()
{
	return g_pluginVersion;
}

const char* CSchematycPlugin::GetPluginName()
{
	return g_szPluginName;
}

const char* CSchematycPlugin::GetPluginDescription()
{
	return g_szPluginDesc;
}


