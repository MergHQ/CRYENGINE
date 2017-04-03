// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoClass.h"

#include "MonoObject.h"
#include "MonoMethod.h"
#include "MonoRuntime.h"

#include "RootMonoDomain.h"

#include <mono/metadata/class.h>
#include <mono/metadata/exception.h>

CMonoClass::CMonoClass(CMonoLibrary* pLibrary, const char *nameSpace, const char *name)
	: m_pLibrary(pLibrary)
	, m_namespace(nameSpace)
	, m_name(name)
{
	m_pClass = mono_class_from_name(pLibrary->GetImage(), nameSpace, name);
}

CMonoClass::CMonoClass(CMonoLibrary* pLibrary, MonoClass* pClass)
	: m_pLibrary(pLibrary)
	, m_pClass(pClass)
{
	m_namespace = mono_class_get_namespace(m_pClass);
	m_name = mono_class_get_name(m_pClass);
}

void CMonoClass::Serialize(CMonoObject* pSerializer)
{
	auto* pDomain = static_cast<CAppDomain*>(m_pLibrary->GetDomain());

	for (auto it = m_objects.begin(); it != m_objects.end();)
	{
		if (std::shared_ptr<CMonoObject> pObject = it->lock())
		{
			pDomain->SerializeObject(pSerializer, (MonoObject*)pObject->GetHandle(), false);

			++it;
		}
		else
		{
			// Clear out dead objects while we're at it
			it = m_objects.erase(it);
		}
	}

	for (auto it = m_methods.begin(); it != m_methods.end();)
	{
		if (std::shared_ptr<CMonoMethod> pMethod = it->lock())
		{
			pMethod->PrepareForSerialization();

			++it;
		}
		else
		{
			// Clear out dead methods while we're at it
			it = m_methods.erase(it);
		}
	}
}

void CMonoClass::Deserialize(CMonoObject* pSerializer)
{
	auto* pDomain = static_cast<CAppDomain*>(m_pLibrary->GetDomain());

	// Deserialize objects from memory
	for (const std::weak_ptr<CMonoObject>& weakObjectPointer : m_objects)
	{
		if (std::shared_ptr<CMonoObject> pObject = weakObjectPointer.lock())
		{
			std::shared_ptr<IMonoObject> pDeserializedObject = pDomain->DeserializeObject(pSerializer, false);
			pObject->AssignObject((MonoObject*)pDeserializedObject->GetHandle());
		}
	}

	// Refresh method pointers, or invalidate them
	for (const std::weak_ptr<CMonoMethod>& weakMethodPointer : m_methods)
	{
		if (std::shared_ptr<CMonoMethod> pMethod = weakMethodPointer.lock())
		{
			// Create an internal mono representation of the description that we saved
			if (MonoMethodDesc* pMethodDesc = mono_method_desc_new(pMethod->GetSerializedDescription(), false))
			{
				// Search for the method signature in all implemented classes
				MonoClass *pClass = m_pClass;

				while (pClass != nullptr)
				{
					if (MonoMethod* pFoundMethod = mono_method_desc_search_in_class(pMethodDesc, pClass))
					{
						pMethod->OnDeserialized(pFoundMethod);
						break;
					}

					pClass = mono_class_get_parent(pClass);
					if (pClass == mono_get_object_class())
						break;
				}

				mono_method_desc_free(pMethodDesc);
			}
		}
	}
}

void CMonoClass::ReloadClass()
{
	m_pClass = mono_class_from_name(m_pLibrary->GetImage(), m_namespace, m_name);
}

void CMonoClass::OnAssemblyUnload()
{
	for (const std::weak_ptr<CMonoObject>& weakObjectPointer : m_objects)
	{
		if (std::shared_ptr<CMonoObject> pObject = weakObjectPointer.lock())
		{
			pObject->ReleaseGCHandle();
		}
	}
}

const char* CMonoClass::GetName() const
{
	return mono_class_get_name(m_pClass);
}

const char* CMonoClass::GetNamespace() const
{
	return mono_class_get_namespace(m_pClass);
}

IMonoAssembly* CMonoClass::GetAssembly() const
{
	return m_pLibrary;
}

std::shared_ptr<IMonoObject> CMonoClass::CreateUninitializedInstance()
{
	auto pWrappedObject = std::make_shared<CMonoObject>(mono_object_new((MonoDomain*)static_cast<CMonoDomain*>(m_pLibrary->GetDomain())->GetHandle(), m_pClass), m_pThis.lock());

	// Push back a weak pointer to the instance, so that we can serialize the object in case of app domain reload
	m_objects.push_back(pWrappedObject);

	return pWrappedObject;
}

std::shared_ptr<IMonoObject> CMonoClass::CreateInstance(void** pConstructorParams, int numParams)
{
	auto* pObject = mono_object_new((MonoDomain*)static_cast<CMonoDomain*>(m_pLibrary->GetDomain())->GetHandle(), m_pClass);

	if (pConstructorParams != nullptr)
	{
		MonoMethod* pConstructorMethod = mono_class_get_method_from_name(m_pClass, ".ctor", numParams);

		bool bException = false;
		CMonoMethod method(pConstructorMethod);
		method.InvokeInternal(pObject, pConstructorParams, bException);

		if (bException)
		{
			return nullptr;
		}
	}
	else
	{
		// Execute the default constructor
		mono_runtime_object_init(pObject);
	}

	auto pWrappedObject = std::make_shared<CMonoObject>(pObject, m_pThis.lock());

	// Push back a weak pointer to the instance, so that we can serialize the object in case of app domain reload
	m_objects.push_back(pWrappedObject);

	return pWrappedObject;
}

std::shared_ptr<IMonoObject> CMonoClass::CreateInstanceWithDesc(const char* parameterDesc, void** pConstructorParams)
{
	auto* pObject = mono_object_new((MonoDomain*)static_cast<CMonoDomain*>(m_pLibrary->GetDomain())->GetHandle(), m_pClass);

	string sMethodDesc;
	sMethodDesc.Format(":.ctor(%s)", parameterDesc);

	auto* pMethodDesc = mono_method_desc_new(sMethodDesc, false);

	if (MonoMethod* pConstructorMethod = mono_method_desc_search_in_class(pMethodDesc, m_pClass))
	{
		mono_method_desc_free(pMethodDesc);

		bool bException = false;
		CMonoMethod method(pConstructorMethod);
		method.InvokeInternal(pObject, pConstructorParams, bException);

		if (bException)
		{
			return nullptr;
		}

		auto pWrappedObject = std::make_shared<CMonoObject>(pObject, m_pThis.lock());

		// Push back a weak pointer to the instance, so that we can serialize the object in case of app domain reload
		m_objects.push_back(pWrappedObject);

		return pWrappedObject;
	}

	mono_method_desc_free(pMethodDesc);

	GetMonoRuntime()->HandleException((MonoObject*)mono_get_exception_missing_method(GetName(), sMethodDesc));
	return nullptr;
}

void CMonoClass::RegisterObject(std::weak_ptr<CMonoObject> pObject)
{
	m_objects.push_back(pObject);
}

std::shared_ptr<IMonoMethod> CMonoClass::FindMethod(const char* szName, int numParams)
{
	if (MonoMethod* pMethod = mono_class_get_method_from_name(m_pClass, szName, numParams))
	{
		auto pMonoMethod = std::make_shared<CMonoMethod>(pMethod);
		m_methods.push_back(pMonoMethod);
		return pMonoMethod;
	}

	return nullptr;
}

std::shared_ptr<IMonoMethod> CMonoClass::FindMethodInInheritedClasses(const char* szName, int numParams)
{
	MonoClass* pClass = m_pClass;
	while (pClass != nullptr)
	{
		if (MonoMethod* pMethod = mono_class_get_method_from_name(pClass, szName, numParams))
		{
			auto pMonoMethod = std::make_shared<CMonoMethod>(pMethod);
			m_methods.push_back(pMonoMethod);
			return pMonoMethod;
		}

		pClass = mono_class_get_parent(pClass);
	}

	return nullptr;
}

std::shared_ptr<IMonoMethod> CMonoClass::FindMethodWithDesc(const char* szMethodDesc)
{
	auto* pMethodDesc = mono_method_desc_new(szMethodDesc, true);
	if (pMethodDesc == nullptr)
	{
		return nullptr;
	}

	if (MonoMethod* pMethod = mono_method_desc_search_in_class(pMethodDesc, m_pClass))
	{
		mono_method_desc_free(pMethodDesc);

		auto pMonoMethod = std::make_shared<CMonoMethod>(pMethod);
		m_methods.push_back(pMonoMethod);
		return pMonoMethod;
	}

	mono_method_desc_free(pMethodDesc);
	return nullptr;
}

std::shared_ptr<IMonoMethod> CMonoClass::FindMethodWithDescInInheritedClasses(const char* szMethodDesc, IMonoClass* pBaseClass)
{
	MonoClass *pClass = m_pClass;

	string sMethodDesc;
	sMethodDesc.Format(":%s", szMethodDesc);

	MonoMethodDesc* pDesiredDesc = mono_method_desc_new(sMethodDesc, false);
	if (pDesiredDesc == nullptr)
	{
		return nullptr;
	}

	while (pClass != nullptr)
	{
		if (MonoMethod* pMethod = mono_method_desc_search_in_class(pDesiredDesc, pClass))
		{
			mono_method_desc_free(pDesiredDesc);

			auto pMonoMethod = std::make_shared<CMonoMethod>(pMethod);
			m_methods.push_back(pMonoMethod);
			return pMonoMethod;
		}

		pClass = mono_class_get_parent(pClass);
		if (pClass == mono_get_object_class() || pClass == pBaseClass->GetHandle())
			break;
	}

	mono_method_desc_free(pDesiredDesc);
	return nullptr;
}