// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoObject.h"

#include "MonoClass.h"
#include "MonoRuntime.h"

#include "AppDomain.h"

#include <CryMono/IMonoAssembly.h>

CMonoObject::CMonoObject(MonoObject* pObject, std::shared_ptr<CMonoClass> pClass)
	: m_pClass(pClass)
	, m_pObject(pObject)
{
	CRY_ASSERT_MESSAGE(m_pClass.get() != nullptr, "SetWeakPointer must be called if no class is available!");

	m_gcHandle = mono_gchandle_new(m_pObject, true);
}

CMonoObject::CMonoObject(MonoObject* pObject)
	: m_pObject(pObject)
{
	m_gcHandle = mono_gchandle_new(m_pObject, true);
}

CMonoObject::~CMonoObject()
{
	ReleaseGCHandle();
}

void CMonoObject::ReleaseGCHandle()
{
	if (m_gcHandle != 0)
	{
		mono_gchandle_free(m_gcHandle);
		m_gcHandle = 0;
	}
}

const char* CMonoObject::ToString() const
{
	MonoObject* pException = nullptr;

	MonoString* pStr = mono_object_to_string(m_pObject, &pException);
	if (pException != nullptr)
	{
		GetMonoRuntime()->HandleException(pException);
		return nullptr;
	}

	if (!pStr)
	{
		return nullptr;
	}

	return mono_string_to_utf8(pStr);
}

size_t CMonoObject::GetArraySize() const
{
	return mono_array_length((MonoArray*)m_pObject);
}

char* CMonoObject::GetArrayAddress(size_t elementSize, size_t index) const
{
	return mono_array_addr_with_size((MonoArray*)m_pObject, elementSize, index);
}

void CMonoObject::AssignObject(MonoObject* pObject)
{
	m_pObject = pObject;
	m_gcHandle = mono_gchandle_new(m_pObject, true);
}

IMonoClass* CMonoObject::GetClass()
{
	if (m_pClass == nullptr)
	{
		MonoClass* pClass = mono_object_get_class(m_pObject);
		MonoImage* pImage = mono_class_get_image(pClass);
		MonoAssembly* pAssembly = mono_image_get_assembly(pImage);

		CMonoLibrary* pLibrary = static_cast<CMonoDomain*>(GetMonoRuntime()->GetActiveDomain())->GetLibraryFromMonoAssembly(pAssembly);

		m_pClass = pLibrary->GetClassFromMonoClass(pClass);
	}

	return m_pClass.get();
}