// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <IResourceSelectorHost.h>
#include <QtViewPane.h>
#include <CryCore/Platform/platform_impl.inl>
#include <IEditorClassFactory.h>
#include <Schematyc/Schematyc_IFramework.h>

#include "Schematyc_CryLinkCommands.h"
#include "Schematyc_MainWindow.h"
#include "Schematyc_Plugin.h"

IEditor* g_pEditor = nullptr;
HINSTANCE g_hInstance = 0;

SCHEMATYC_PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return NULL;
	}

	g_pEditor = pInitParam->pIEditor;
	ModuleInitISystem(g_pEditor->GetSystem(), "Schematyc_Plugin");

	if (!GetSchematycFrameworkPtr())
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return NULL;
	}

	RegisterPlugin();
	RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelectorHost());

	Schematyc::CCryLinkCommands::GetInstance().Register(g_pEditor->GetSystem()->GetIConsole());

	return new CSchematycPlugin;
}

// Force MFC to use this DllMain.
#ifdef _USRDLL
extern "C"
{
	int __afxForceUSRDLL;
}
#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		g_hInstance = hinstDLL;
	}
	return TRUE;
}

IEditor* GetIEditor()
{
	return g_pEditor;
}

HINSTANCE GetHInstance()
{
	return g_hInstance;
}
