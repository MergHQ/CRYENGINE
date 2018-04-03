// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include <CryCore/Platform/platform_impl.inl>

#include <Include/IEditorClassFactory.h>

#include "Editor/MonoDevelop_Plugin.h"

IEditor* g_pEditor = 0;
HINSTANCE g_hInstance = 0;

IEditor* GetIEditor()
{
	return g_pEditor;
}

PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}

	g_pEditor = pInitParam->pIEditorInterface;
	ModuleInitISystem(GetIEditor()->GetSystem(), "MonoDevelopPlugin");
	return new CMonoDevelopPlugin();
}
