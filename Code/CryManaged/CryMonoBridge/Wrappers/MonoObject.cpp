// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoObject.h"

#include "MonoClass.h"
#include "MonoRuntime.h"

#include "AppDomain.h"

#include <CryMono/IMonoAssembly.h>

CMonoObject::CMonoObject(MonoObject* pObject, std::shared_ptr<IMonoClass> pClass)
	: m_pClass(pClass)
	, m_pObject(pObject)
{
	m_gcHandle = mono_gchandle_new(m_pObject, true);
}

CMonoObject::~CMonoObject()
{
	mono_gchandle_free(m_gcHandle);
}

std::shared_ptr<IMonoObject> CMonoObject::InvokeMethod(const char *methodName, void **pParams, int numParams) const
{
	return m_pClass->InvokeMethod(methodName, this, pParams, numParams);
}

std::shared_ptr<IMonoObject> CMonoObject::InvokeMethodWithDesc(const char* methodDesc, void** pParams) const
{
	return m_pClass->InvokeMethodWithDesc(methodDesc, this, pParams);
}

const char* CMonoObject::ToString() const
{
	MonoObject* pException = nullptr;

	MonoString* pStr = mono_object_to_string(m_pObject, &pException);
	if (pException != nullptr)
	{
		static_cast<CMonoRuntime*>(gEnv->pMonoRuntime)->HandleException(pException);
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

void CMonoObject::Serialize()
{
	auto* pDomain = static_cast<CAppDomain*>(m_pClass->GetAssembly()->GetDomain());

	pDomain->Serialize(m_pObject, false);
}

void CMonoObject::Deserialize()
{
	auto* pDomain = static_cast<CAppDomain*>(m_pClass->GetAssembly()->GetDomain());

	auto pDeserializedObject = pDomain->Deserialize(false);
	
	m_pObject = (MonoObject*)pDeserializedObject->GetHandle();
	m_gcHandle = mono_gchandle_new(m_pObject, true);
}