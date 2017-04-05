// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.
// Sandbox plugin wrapper.
#include "StdAfx.h"
#include "SandboxPlugin.h"
#include "MainDialog.h"
#include "QtViewPane.h"
#include "IEditor.h"
#include <CrySystem/ISystem.h>
#include <EditorFramework/PersonalizationManager.h>

// Plugin versioning
static const char* g_szPluginName = "MeshImporter";
static const char* g_szPluginGuid = "{4F68F679-39AD-4BB4-B5E2-8A8DD3020DC1}";
static DWORD g_pluginVersion = 1;

// Plugin instance
static CFbxToolPlugin* g_pInstance = nullptr;

// Global lock instance
CryCriticalSection Detail::g_lock;

CFbxToolPlugin::CFbxToolPlugin(IEditor* pEditor)
	: m_pEditor(pEditor)
{
	CScopedGlobalLock lock;

	assert(g_pInstance == nullptr);
	g_pInstance = this;

	RegisterPlugin();
}

void CFbxToolPlugin::Release()
{
	CScopedGlobalLock lock;

	UnregisterPlugin();

	assert(g_pInstance == this);
	g_pInstance = nullptr;
	delete this;
}

CFbxToolPlugin* CFbxToolPlugin::GetInstance()
{
	return g_pInstance;
}

const char* CFbxToolPlugin::GetPluginName()
{
	return g_szPluginName;
}

const char* CFbxToolPlugin::GetPluginGUID()
{
	return g_szPluginGuid;
}

DWORD CFbxToolPlugin::GetPluginVersion()
{
	return g_pluginVersion;
}

// Requirement for Sandbox plugin
IEditor* GetIEditor()
{
	CFbxToolPlugin* const pPlugin = CFbxToolPlugin::GetInstance();
	assert(pPlugin);
	IEditor* const pEditor = pPlugin->GetIEditor();
	assert(pEditor);
	return pEditor;
}

static void LogVPrintf(const char* szFormat, va_list args)
{
	string format;
	format.Format("[%s] %s", g_szPluginName, szFormat);
	gEnv->pLog->LogV(IMiniLog::eMessage, format.c_str(), args);
}

void LogPrintf(const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	LogVPrintf(szFormat, args);
	va_end(args);
}

namespace FbxTool
{
namespace Detail
{

void Log(const char* szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	LogVPrintf(szFormat, args);
	va_end(args);
}

} // namespace Detail
} // namespace FbxTool

void CFbxToolPlugin::SetPersonalizationProperty(const QString& propName, const QVariant& value)
{
	GetIEditor()->GetPersonalizationManager()->SetProperty(MESH_IMPORTER_NAME, propName, value);
}

const QVariant& CFbxToolPlugin::GetPersonalizationProperty(const QString& propName)
{
	return GetIEditor()->GetPersonalizationManager()->GetProperty(MESH_IMPORTER_NAME, propName);
}

// Entry point for plugin initializer
PLUGIN_API IPlugin* CreatePluginInstance(PLUGIN_INIT_PARAM* pInitParam)
{
	if (pInitParam->pluginVersion != SANDBOX_PLUGIN_SYSTEM_VERSION)
	{
		return nullptr;
	}

	// Initialize globals
	CScopedGlobalLock lock;
	IEditor* const pEditor = pInitParam->pIEditor;

	// Initialize module
	ModuleInitISystem(pEditor->GetSystem(), g_szPluginName);

	// Register cvars.
	REGISTER_STRING("mi_defaultMaterial", "EngineAssets/TextureMsg/DefaultSolids", 0, "Default material.");
	REGISTER_INT("mi_lazyLodGeneration", 1, 0, "When non-zero, LOD auto-generation is deferred until LODs are actually visible.");

	// Initialize plugin
	return new CFbxToolPlugin(pEditor);
}

#include <CryCore/Platform/platform_impl.inl>
