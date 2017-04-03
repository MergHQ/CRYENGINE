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

CMonoLibrary::CMonoLibrary(const char* filePath, CMonoDomain* pDomain)
	: CMonoLibrary(nullptr, filePath, pDomain)
{
	Load();
}

CMonoLibrary::CMonoLibrary(MonoAssembly* pAssembly, const char* filePath, CMonoDomain* pDomain)
	: m_pAssembly(pAssembly)
	, m_pDomain(pDomain)
	, m_assemblyPath(filePath)
	, m_pImage(pAssembly != nullptr ? mono_assembly_get_image(pAssembly) : nullptr)
{
}

CMonoLibrary::~CMonoLibrary()
{
	// Destroy classes before closing the assembly
	m_classes.clear();

	Unload();
}

bool CMonoLibrary::Load()
{
// Loading with CryPak disabled for now, need to make sure that chain-reload works correctly and that images are closed.
//#define LOAD_MONO_LIBRARIES_WITH_CRYPAK
#ifdef LOAD_MONO_LIBRARIES_WITH_CRYPAK
	// Now load from disk
	CCryFile file;
	file.Open(m_assemblyPath, "rb", ICryPak::FLAGS_PATH_REAL);

	if (file.GetHandle() == nullptr)
	{
		// Search in additional binary directories
		for (const string& binaryDirectory : GetMonoRuntime()->GetBinaryDirectories())
		{
			m_assemblyPath = PathUtil::Make(binaryDirectory, m_assemblyPath);

			file.Open(m_assemblyPath, "rb", ICryPak::FLAGS_PATH_REAL);
			if (file.GetHandle() != nullptr)
			{
				break;
			}
		}

		if (file.GetHandle() == nullptr)
		{
			return nullptr;
		}
	}

	m_assemblyData.resize(file.GetLength());
	file.SeekToBegin();
	file.ReadType(m_assemblyData.data(), m_assemblyData.size());

	MonoImageOpenStatus status = MONO_IMAGE_ERROR_ERRNO;

	// Load the assembly image
	m_pImage = mono_image_open_from_data_with_name(m_assemblyData.data(), m_assemblyData.size(), false, &status, false, nullptr);

	// Make sure to load references this image requires before loading the assembly itself
	for (int i = 0; i < mono_image_get_table_rows(m_pImage, MONO_TABLE_ASSEMBLYREF); ++i)
	{
		mono_assembly_load_reference(m_pImage, i);
	}

	gEnv->pLog->LogWithType(IMiniLog::eMessage, "[Mono] Load Library: %s", m_assemblyPath.c_str());

#ifndef _RELEASE
	// load debug information
	string sDebugDatabasePath = m_assemblyPath;
	sDebugDatabasePath.append(".mdb");

	CCryFile debugFile;
	debugFile.Open(sDebugDatabasePath, "rb", ICryPak::FLAGS_PATH_REAL);
	if (debugFile.GetHandle() != nullptr)
	{
		m_assemblyDebugData.resize(debugFile.GetLength());
		debugFile.SeekToBegin();
		debugFile.ReadType(m_assemblyDebugData.data(), m_assemblyDebugData.size());

		mono_debug_open_image_from_memory(m_pImage, m_assemblyDebugData.data(), m_assemblyPath.size());
	}
#endif

	m_pAssembly = mono_assembly_load_from_full(m_pImage, m_assemblyPath.c_str(), &status, false);
	
	return m_pAssembly != nullptr;
#else

	string assemblyPath = m_assemblyPath;

	// Do a copy of the binary on windows to allow reload during development
	// Otherwise Mono will lock the file.
#if defined(CRY_PLATFORM_WINDOWS) && !defined(RELEASE)
	TCHAR tempPath[MAX_PATH];
	GetTempPathA(MAX_PATH, tempPath);

	string tempBinaryDirectory = PathUtil::Make(tempPath, "CE_ManagedBinaries\\");

	DWORD attribs = GetFileAttributesA(tempBinaryDirectory.c_str());
	if (attribs == INVALID_FILE_ATTRIBUTES)
	{
		CryCreateDirectory(tempBinaryDirectory.c_str());
	}

	string fileName = PathUtil::GetFile(m_assemblyPath);
	assemblyPath = PathUtil::Make(tempBinaryDirectory, fileName);

	gEnv->pCryPak->CopyFileOnDisk(m_assemblyPath, assemblyPath, false);
#endif

	m_pAssembly = mono_domain_assembly_open((MonoDomain*)m_pDomain->GetHandle(), assemblyPath);
	
	if (m_pAssembly != nullptr)
	{
		m_pImage = mono_assembly_get_image(m_pAssembly);

		return true;
	}

	return false;
#endif
}

void CMonoLibrary::Unload()
{
	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->OnAssemblyUnload();
	}
}

void CMonoLibrary::Reload()
{
	Load();

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->ReloadClass();
	}
}

void CMonoLibrary::Serialize(CMonoObject* pSerializer)
{
	// Make sure we know the file path to this binary, we'll need to reload it later.
	// This is automatically cached in GetFilePath.
	GetFilePath();

	static_cast<CAppDomain*>(m_pDomain)->SerializeObject(pSerializer, GetManagedObject(), true);

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->Serialize(pSerializer);
	}
}

void CMonoLibrary::Deserialize(CMonoObject* pSerializer)
{
	static_cast<CAppDomain*>(m_pDomain)->DeserializeObject(pSerializer, true);

	for (auto it = m_classes.begin(); it != m_classes.end(); ++it)
	{
		it->get()->Deserialize(pSerializer);
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
	MonoException *pException;
	if (message != nullptr)
		pException = mono_exception_from_name_msg(m_pImage, nameSpace, exceptionClass, message);
	else
		pException = mono_exception_from_name(m_pImage, nameSpace, exceptionClass);

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
	m_classes.emplace_back(pClass);

	return pClass;
}

MonoObject* CMonoLibrary::GetManagedObject()
{
	return (MonoObject*)mono_assembly_get_object((MonoDomain*)m_pDomain->GetHandle(), m_pAssembly);
}


const char* CMonoLibrary::GetImageName() const
{
	return mono_image_get_name(m_pImage);
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