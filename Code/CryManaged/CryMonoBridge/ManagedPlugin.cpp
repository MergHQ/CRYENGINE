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
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this,"CManagedPlugin");
}

CManagedPlugin::~CManagedPlugin()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);

	if (m_pMonoObject != nullptr)
	{
		m_pMonoObject->GetClass()->FindMethod("Shutdown")->Invoke(m_pMonoObject.get());
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
		CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to get class %s:%s for plug-in!", nameSpace.c_str(), m_pluginName.c_str());
		return false;
	}

	m_pMonoObject = std::static_pointer_cast<CMonoObject>(m_pClass->CreateInstance());
	if (m_pMonoObject == nullptr)
	{
		return false;
	}

	auto pEngineClass = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine", "Engine");
	
	void* pRegisterArgs[1];
	pRegisterArgs[0] = m_pLibrary->GetManagedObject();

	pEngineClass->FindMethodWithDesc(":ScanAssembly(System.Reflection.Assembly)")->InvokeStatic(pRegisterArgs);

	m_pMonoObject->GetClass()->FindMethod("Initialize")->Invoke(m_pMonoObject.get());

	return true;
}

const char* CManagedPlugin::TryGetPlugin() const
{
	auto pReflectionHelper = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine", "ReflectionHelper");
	CRY_ASSERT(pReflectionHelper != nullptr);

	void* args[1];
	args[0] = m_pLibrary->GetManagedObject();

	auto pReturnValue = pReflectionHelper->FindMethodWithDesc("ReflectionHelper:FindPluginInstance(System.Reflection.Assembly)")->InvokeStatic(args);
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
		auto* pPluginDomain = GetMonoRuntime()->LaunchPluginDomain();
		CRY_ASSERT(pPluginDomain != nullptr);

		m_pLibrary = pPluginDomain->LoadLibrary(m_libraryPath);
		if (m_pLibrary == nullptr)
		{
			gEnv->pLog->LogError("[Mono] could not initialize plugin '%s'!", m_libraryPath.c_str());
		}
		else
		{
			bool pluginInitialized = InitializePlugin();
			if (pluginInitialized)
			{
				// scans for console command attributes
				std::shared_ptr<IMonoClass> attributeManager = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Attributes", "ConsoleCommandAttributeManager");
				CRY_ASSERT(attributeManager != nullptr);
				void* args[1];
				args[0] = m_pLibrary->GetManagedObject(); //load the plugin assembly to scans for ConsoleCommandRegisterAttribute
				attributeManager->FindMethod("RegisterAttribute", 1)->Invoke(nullptr, args);

				//scans for console variable attributes
				std::shared_ptr<IMonoClass> consoleVariableAttributeManager = GetMonoRuntime()->GetCryCoreLibrary()->GetTemporaryClass("CryEngine.Attributes", "ConsoleVariableAttributeManager");
				CRY_ASSERT(consoleVariableAttributeManager != nullptr);
				void* args2[1];
				args2[0] = m_pLibrary->GetManagedObject(); //load the plugin assembly to scans for ConsoleVariableAttribute
				consoleVariableAttributeManager->FindMethod("RegisterAttribute", 1)->Invoke(nullptr, args2);
			}
		}
	}
	break;
	case ESYSTEM_EVENT_LEVEL_LOAD_END:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->GetClass()->FindMethod("OnLevelLoaded")->Invoke(m_pMonoObject.get());
			}
		}
		break;
	case ESYSTEM_EVENT_EDITOR_GAME_MODE_CHANGED:
		{
			if (wparam == 1)
			{
				if (m_pMonoObject != nullptr)
				{
					m_pMonoObject->GetClass()->FindMethod("OnGameStart")->Invoke(m_pMonoObject.get());
				}
			}
			else if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->GetClass()->FindMethod("OnGameStop")->Invoke(m_pMonoObject.get());
			}
		}
		break;
	case ESYSTEM_EVENT_LEVEL_GAMEPLAY_START:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->GetClass()->FindMethod("OnGameStart")->Invoke(m_pMonoObject.get());
			}
		}
		break;
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
		{
			if (m_pMonoObject != nullptr)
			{
				m_pMonoObject->GetClass()->FindMethod("OnGameStop")->Invoke(m_pMonoObject.get());
			}
		}
		break;
	}
}