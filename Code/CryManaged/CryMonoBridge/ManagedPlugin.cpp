// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ManagedPlugin.h"

#include "MonoRuntime.h"
#include "Wrappers/AppDomain.h"
#include "Wrappers/MonoLibrary.h"

#include <CryMono/IMonoAssembly.h>
#include <CryMono/IMonoClass.h>

CManagedPlugin::CManagedPlugin(const char* szBinaryPath)
	: m_pLibrary(nullptr)
	, m_pClass(nullptr)
	, m_libraryPath(szBinaryPath)
	, m_pMonoObject(nullptr)
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
}

CManagedPlugin::~CManagedPlugin()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (m_pMonoObject != nullptr)
	{
		m_pMonoObject->InvokeMethod("Shutdown");
	}
}

bool CManagedPlugin::InitializePlugin()
{
	const char* pluginClassName = TryGetPlugin();
	if (pluginClassName == nullptr || strlen(pluginClassName) == 0)
	{
		return false;
	}

	m_pluginName = PathUtil::GetExt(pluginClassName);
	const string nameSpace = PathUtil::RemoveExtension(pluginClassName);

	m_pClass = m_pLibrary->GetClass(nameSpace, m_pluginName);
	if (m_pClass == nullptr)
	{
		CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to get class %s:%s for plugin!", nameSpace.c_str(), m_pluginName.c_str());
		return false;
	}

	m_pMonoObject = m_pClass->CreateInstance();

	if (!m_pMonoObject)
	{
		return false;
	}

	auto pEngineClass = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetTemporaryClass("CryEngine", "Engine");
	
	void* pRegisterArgs[1];
	pRegisterArgs[0] = m_pLibrary->GetManagedObject();

	pEngineClass->InvokeMethodWithDesc(":ScanAssembly(System.Reflection.Assembly)", nullptr, pRegisterArgs);

	m_pMonoObject->InvokeMethod("Initialize");

	return true;
}

const char* CManagedPlugin::TryGetPlugin() const
{
	auto pReflectionHelper = gEnv->pMonoRuntime->GetCryCoreLibrary()->GetTemporaryClass("CryEngine", "ReflectionHelper");
	CRY_ASSERT(pReflectionHelper != nullptr);

	void* args[1];
	args[0] = m_pLibrary->GetManagedObject();

	auto pReturnValue = pReflectionHelper->InvokeMethodWithDesc("ReflectionHelper:FindPluginInstance(System.Reflection.Assembly)", nullptr, args);
	if (pReturnValue == nullptr)
	{
		return nullptr;
	}

	return pReturnValue->ToString();
}

void CManagedPlugin::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	switch (event)
	{
	case ESYSTEM_EVENT_GAME_POST_INIT:
	{
		// Make sure the plugin domain exists, since plugins are always loaded in there
		auto* pPluginDomain = static_cast<CMonoRuntime*>(gEnv->pMonoRuntime)->LaunchPluginDomain();
		CRY_ASSERT(pPluginDomain != nullptr);

		m_pLibrary = pPluginDomain->LoadLibrary(m_libraryPath);
		if (m_pLibrary == nullptr)
		{
			gEnv->pLog->LogError("[Mono] could not initialize plugin '%s'!", m_libraryPath.c_str());
		}
		else
		{
			InitializePlugin();
		}
	}
	break;
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->InvokeMethod("OnLevelLoaded");
			}
		}
		break;
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
		{
			if (wparam == 1)
			{
				if (m_pMonoObject != nullptr)
				{
					m_pMonoObject->InvokeMethod("OnGameStart");
				}
			}
			else if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->InvokeMethod("OnGameStop");
			}
		}
		break;
	case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->InvokeMethod("OnGameStart");
			}
		}
		break;
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->InvokeMethod("OnGameStop");
			}
		}
		break;
	}
}