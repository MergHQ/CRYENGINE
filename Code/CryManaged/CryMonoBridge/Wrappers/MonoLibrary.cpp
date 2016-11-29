// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoLibrary.h"

#include "MonoRuntime.h"
#include "MonoDomain.h"
#include "MonoClass.h"
#include "MonoObject.h"
#include "MonoException.h"

#include "AppDomain.h"
#include "RootMonoDomain.h"

#include <mono/metadata/object.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/exception.h>

CMonoLibrary::CMonoLibrary(MonoAssembly* pAssembly, const char* filePath, CMonoDomain* pDomain, char* data)
	: m_pAssembly(pAssembly)
	, m_pDomain(pDomain)
	, m_pMemory(data)
	, m_pMemoryDebug(nullptr)
	, m_assemblyPath(filePath)
{
}

CMonoLibrary::~CMonoLibrary()
{
	// Destroy classes before closing the assembly
	m_classes.clear();

	Unload();
}

void CMonoLibrary::Unload()
{
	if (m_pAssembly && m_pMemoryDebug != nullptr)
	{
		mono_debug_close_image(mono_assembly_get_image(m_pAssembly));
		SAFE_DELETE_ARRAY(m_pMemoryDebug);
	}

	SAFE_DELETE_ARRAY(m_pMemory);
}

void CMonoLibrary::Reload()
{
	FILE* pFile = gEnv->pCryPak->FOpen(m_assemblyPath, "rb", ICryPak::FLAGS_PATH_REAL);

	m_pAssembly = m_pDomain->LoadMonoAssembly(m_assemblyPath, pFile, &m_pMemory, &m_pMemoryDebug);

	gEnv->pCryPak->FClose(pFile);

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->ReloadClass();
	}
}

void CMonoLibrary::Serialize()
{
	// Make sure we know the file path to this binary, we'll need to reload it later.
	// This is automatically cached in GetFilePath.
	GetFilePath();

	static_cast<CAppDomain*>(m_pDomain)->Serialize(GetManagedObject(), true);

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->Serialize();
	}
}

void CMonoLibrary::Deserialize()
{
	static_cast<CAppDomain*>(m_pDomain)->Deserialize(true);

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->Deserialize();
	}
}

IMonoClass* CMonoLibrary::GetClass(const char *nameSpace, const char *className)
{
	m_classes.emplace_back(std::make_shared<CMonoClass>(this, nameSpace, className));

	auto pClass = m_classes.back();
	pClass->SetWeakPointer(pClass);

	return pClass.get();
}

std::shared_ptr<IMonoClass> CMonoLibrary::GetTemporaryClass(const char *nameSpace, const char *className)
{
	auto pClass = std::make_shared<CMonoClass>(this, nameSpace, className);

	// Since this class may expire at any time the class has to contain a weak pointer of itself
	// This is converted to a shared_ptr when the class creates objects, to ensure that the class always outlives its instances
	pClass->SetWeakPointer(pClass);

	return pClass;
}

std::shared_ptr<IMonoException> CMonoLibrary::GetExceptionInternal(const char* nameSpace, const char* exceptionClass, const char* message)
{
	MonoImage* pImage = mono_assembly_get_image(m_pAssembly);

	MonoException *pException;
	if (message != nullptr)
		pException = mono_exception_from_name_msg(pImage, nameSpace, exceptionClass, message);
	else
		pException = mono_exception_from_name(pImage, nameSpace, exceptionClass);

	return std::make_shared<CMonoException>(pException);
}

std::shared_ptr<CMonoClass> CMonoLibrary::GetClassFromMonoClass(MonoClass* pMonoClass)
{
	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		if (it->get()->GetHandle() == pMonoClass)
		{
			return *it;
		}
	}

	auto pClass = std::make_shared<CMonoClass>(this, pMonoClass);

	pClass->SetWeakPointer(pClass);

	return pClass;
}

MonoObject* CMonoLibrary::GetManagedObject()
{
	return (MonoObject*)mono_assembly_get_object((MonoDomain*)m_pDomain->GetHandle(), m_pAssembly);
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

const char* CMonoLibrary::GetFilePath()
{
	if (m_assemblyPath.size() == 0)
	{
		MonoAssemblyName* pAssemblyName = mono_assembly_get_name(m_pAssembly);
		m_assemblyPath = mono_assembly_name_get_name(pAssemblyName);
	}

	return m_assemblyPath;
}

IMonoDomain* CMonoLibrary::GetDomain() const
{
	return m_pDomain;
}

MonoImage* CMonoLibrary::GetImage() const
{ 
	return mono_assembly_get_image(m_pAssembly); 
}