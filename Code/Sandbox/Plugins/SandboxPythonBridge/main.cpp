// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include <CryCore/Platform/platform_impl.inl>
#include "SandboxPythonBridgePlugin.h"

IEditor* g_pEditor = nullptr;
SandboxPythonBridgePlugin* g_pPlugin = nullptr;

DLL_EXPORT IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		pInitParam->outErrorCode = IPlugin::eError_VersionMismatch;
		return 0;
	}

	g_pEditor = pInitParam->pIEditor;
	g_pPlugin = new SandboxPythonBridgePlugin();
	return g_pPlugin;
}

IEditor* GetIEditor()
{
	return g_pEditor;
}

SandboxPythonBridgePlugin* GetSandboxPythonBridge()
{
	return g_pPlugin;
}