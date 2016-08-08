// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoLibrary.h"
#include "MonoRuntime.h"

#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>

CMonoLibrary::CMonoLibrary(MonoAssembly* pAssembly, char* data)
	: m_pAssembly(pAssembly)
	, m_pMemory(data)
	, m_pMemoryDebug(nullptr)
	, m_pMonoClass(nullptr)
	, m_pMonoObject(nullptr)
	, m_gcHandle(0)
{}

CMonoLibrary::~CMonoLibrary()
{
	CloseDebug();
	Close();
}

ICryEngineBasePlugin* CMonoLibrary::Initialize(void* pMonoDomain, void* pDomainHandler)
{
	MonoDomain* pDomain = static_cast<MonoDomain*>(pMonoDomain);
	MonoImage* pImage = mono_assembly_get_image(m_pAssembly);

	const char* szType = TryGetPlugin(pDomain);
	if (szType != nullptr)
	{
		const char* szClassName = PathUtil::GetExt(szType);
		const string nameSpace = PathUtil::RemoveExtension(szType);

		CryLogAlways("class name: %s, name space: %s", szClassName, nameSpace.c_str());

		m_pMonoClass = mono_class_from_name(pImage, nameSpace.c_str(), szClassName);

		if (!m_pMonoClass)
		{
			return nullptr;
		}

		m_pMonoObject = mono_object_new(pDomain, m_pMonoClass);

		if (!m_pMonoObject)
		{
			return nullptr;
		}

		m_gcHandle = mono_gchandle_new(m_pMonoObject, true);

		mono_runtime_object_init(m_pMonoObject);

		MonoClass* pMonoBaseClass = mono_class_get_parent(m_pMonoClass);
		if (pMonoBaseClass)
		{
			MonoMethod* pDomainMethod = mono_class_get_method_from_name(pMonoBaseClass, "set_m_interDomainHandler", 1);
			if (pDomainHandler != nullptr && pDomainMethod)
			{
				void* args[1];
				args[0] = pDomainHandler;
				mono_runtime_invoke(pDomainMethod, m_pMonoObject, args, nullptr);
			}

			MonoClass* pPluginBaseClass = mono_class_get_parent(pMonoBaseClass);
			MonoMethod* pMethod = mono_class_get_method_from_name(pPluginBaseClass, "getCPtr", 1);
			if (pPluginBaseClass && pMethod)
			{
				void* args[1];
				args[0] = m_pMonoObject;
				MonoObject* pResult = mono_runtime_invoke(pMethod, m_pMonoObject, args, nullptr);
				MonoClass* pResultClass = pResult ? mono_object_get_class(pResult) : nullptr;
				MonoProperty* pPropertyHandle = pResultClass ? mono_class_get_property_from_name(pResultClass, "Handle") : nullptr;
				MonoObject* pHandle = pPropertyHandle ? mono_property_get_value(pPropertyHandle, pResult, nullptr, nullptr) : nullptr;
				MonoClass* pHandleClass = pHandle ? mono_object_get_class(pHandle) : nullptr;
				MonoClassField* pHandleClassValueField = pHandleClass ? mono_class_get_field_from_name(pHandleClass, "m_value") : nullptr;
				MonoObject* pValue = pHandleClassValueField ? mono_field_get_value_object(pDomain, pHandleClassValueField, pHandle) : nullptr;
				if (pValue)
				{
					void* pArr3 = mono_array_get((MonoArray*)pValue, void*, 3);

					ICryEngineBasePlugin* pCryEngineBasePlugin = static_cast<ICryEngineBasePlugin*>(pArr3);
					if (pCryEngineBasePlugin && RunMethod("Initialize"))
					{
						return pCryEngineBasePlugin;
					}
				}
			}
		}
	}

	return nullptr;
}

bool CMonoLibrary::RunMethod(const char* szMethodName) const
{
	CRY_ASSERT(m_pAssembly);

	if (m_pMonoClass)
	{
		MonoMethod* pMethod = mono_class_get_method_from_name(m_pMonoClass, szMethodName, 0);
		if (!pMethod)
		{
			gEnv->pLog->LogError("[Mono][RunMethod] Could not find method '%s' in assembly'%s'!", szMethodName, GetImageName());
			return false;
		}

		mono_runtime_invoke(pMethod, m_pMonoObject, nullptr, nullptr);
		return true;
	}

	return false;
}

const char* CMonoLibrary::GetImageName() const
{
	MonoImage* pImage = mono_assembly_get_image(m_pAssembly);
	if (pImage)
	{
		return mono_image_get_name(pImage);
	}

	return nullptr;
}

void CMonoLibrary::Close()
{
	SAFE_DELETE_ARRAY(m_pMemory);
}

void CMonoLibrary::CloseDebug()
{
	if (m_pAssembly)
	{
		mono_debug_close_image(mono_assembly_get_image(m_pAssembly));
		SAFE_DELETE_ARRAY(m_pMemoryDebug);
	}
}

const char* CMonoLibrary::TryGetPlugin(MonoDomain* pDomain) const
{
	MonoImage* pImage = mono_assembly_get_image(m_pAssembly);
	const char* szAsmName = mono_image_get_name(pImage);

	CMonoRuntime* pRuntime = static_cast<CMonoRuntime*>(gEnv->pMonoRuntime);
	CMonoLibrary* pLib = static_cast<CMonoLibrary*>(pRuntime->GetCore());
	if (!pLib)
	{
		return nullptr;
	}

	MonoImage* pImg = mono_assembly_get_image(pLib->GetAssembly());
	if (!pImg)
	{
		return nullptr;
	}

	MonoMethodDesc* pMethodDesc = mono_method_desc_new("CryEngine.ReflectionHelper:FindPluginInstance(string)", true);
	MonoMethod* pMethod = mono_method_desc_search_in_image(pMethodDesc, pImg);
	if (!pMethod)
	{
		return nullptr;
	}

	void* args[1];
	args[0] = mono_string_new(pDomain, szAsmName);

	MonoObject* pReturn = mono_runtime_invoke(pMethod, nullptr, args, nullptr);
	if (!pReturn)
	{
		return nullptr;
	}

	MonoString* pStr = mono_object_to_string(pReturn, nullptr);
	if (!pStr)
	{
		return nullptr;
	}

	return mono_string_to_utf8(pStr);
}

bool SMonoDomainHandlerLibrary::Initialize()
{
	MonoImage* pImage = mono_assembly_get_image(m_pLibrary->GetAssembly());
	MonoMethodDesc* pMethodDesc = mono_method_desc_new("CryEngine.DomainHandler.DomainHandler:GetDomainHandler()", false);
	MonoMethod* pMethod = mono_method_desc_search_in_image(pMethodDesc, pImage);
	m_pDomainHandlerObject = mono_runtime_invoke(pMethod, nullptr, nullptr, nullptr);

	return (m_pDomainHandlerObject != nullptr);
}
