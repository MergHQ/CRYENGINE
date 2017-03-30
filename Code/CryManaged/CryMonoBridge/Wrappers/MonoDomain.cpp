#include "StdAfx.h"
#include "MonoDomain.h"

#include "MonoRuntime.h"
#include "MonoLibrary.h"

#include <mono/jit/jit.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>

#include <CrySystem/IProjectManager.h>

CMonoDomain::CMonoDomain()
{
	char executableFolder[_MAX_PATH];
	CryGetExecutableFolder(_MAX_PATH, executableFolder);

	string sDirectory = executableFolder;
	PathUtil::UnifyFilePath(sDirectory);

	m_binaryDirectories.push_back(sDirectory);
}

CMonoDomain::~CMonoDomain()
{
	// Destroy libraries in reverse order, to resolve dependency issues
	for (auto it = m_loadedLibraries.rbegin(); it != m_loadedLibraries.rend(); ++it)
	{
		// Delete the CMonoLibrary object
		it->pLibrary.reset();
	}

	// Destroy the domain
	Unload();
}

bool CMonoDomain::Activate(bool bForce)
{
	return mono_domain_set(m_pDomain, bForce) != 0;
}

bool CMonoDomain::IsActive() const
{
	return m_pDomain == mono_domain_get(); 
}

MonoString* CMonoDomain::CreateString(const char *text)
{
	return mono_string_new(m_pDomain, text);
}

void CMonoDomain::Unload()
{
	if (!m_bNativeAssembly)
		return;

	if (m_pDomain == mono_get_root_domain())
	{
		mono_jit_cleanup(m_pDomain);
	}
	else
	{
		if (IsActive())
		{
			// Fall back to the root domain
			mono_domain_set(mono_get_root_domain(), true);
		}

		//mono_domain_finalize(m_pDomain, 2000);

		MonoObject *pException;
		try
		{
			mono_domain_try_unload(m_pDomain, &pException);
		}
		catch (char *ex)
		{
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "An exception was raised during AppDomain unload: %s", ex);
		}

		if (pException)
		{
			GetMonoRuntime()->HandleException(pException);
		}
	}
}

CMonoLibrary* CMonoDomain::LoadLibrary(const char* path, bool bRefOnly)
{
	string sFilePath = path;

	if (strcmp("dll", PathUtil::GetExt(sFilePath.c_str())))
	{
		sFilePath.append(".dll");
	}

	// First try loading the direct path passed in
	FILE* pFileHandle = gEnv->pCryPak->FOpen(sFilePath, "rb", ICryPak::FLAGS_PATH_REAL);

	if (pFileHandle == nullptr)
	{
		string binaryFileName = PathUtil::GetFile(sFilePath);

		for (auto it = m_binaryDirectories.begin(); it != m_binaryDirectories.end(); ++it)
		{
			sFilePath = PathUtil::Make(*it, binaryFileName);

			pFileHandle = gEnv->pCryPak->FOpen(sFilePath, "rb", ICryPak::FLAGS_PATH_REAL);
			if (pFileHandle != nullptr)
			{
				break;
			}
		}

		if (pFileHandle == nullptr)
		{
			return nullptr;
		}
	}

	PathUtil::UnifyFilePath(sFilePath);

	for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		if (!strcmp(it->filePath, sFilePath))
		{
			// Might return null here in case Mono callbacks result in recursive load
			return it->pLibrary.get();
		}
	}

	string binaryDirectory = PathUtil::GetPathWithoutFilename(sFilePath);
	stl::push_back_unique(m_binaryDirectories, binaryDirectory);

	// Insert the path early (but pLibrary set to null) since LoadMonoAssembly can result in an attempt to load twice
	auto loadedLibraryIndex = m_loadedLibraries.size();
	m_loadedLibraries.emplace_back(sFilePath);

	char* imageData;
	mono_byte* debugDatabaseData = nullptr;

	if (MonoAssembly* pAssembly = LoadMonoAssembly(sFilePath, pFileHandle, &imageData, &debugDatabaseData, bRefOnly))
	{
		gEnv->pCryPak->FClose(pFileHandle);

		// Pass image data to the library, it has to be deleted when the assembly is done
		m_loadedLibraries[loadedLibraryIndex].pLibrary = std::unique_ptr<CMonoLibrary>(new CMonoLibrary(pAssembly, sFilePath, this, imageData));

	#ifndef _RELEASE
		if (debugDatabaseData != nullptr)
		{
			m_loadedLibraries[loadedLibraryIndex].pLibrary->SetDebugData(debugDatabaseData);
		}
	#endif

		return m_loadedLibraries[loadedLibraryIndex].pLibrary.get();
	}

	gEnv->pCryPak->FClose(pFileHandle);
	return nullptr;
}

CMonoLibrary* CMonoDomain::GetLibraryFromMonoAssembly(MonoAssembly* pAssembly)
{
	for (auto it = m_loadedLibraries.begin(); it != m_loadedLibraries.end(); ++it)
	{
		if (it->pLibrary.get()->GetAssembly() == pAssembly)
		{
			return it->pLibrary.get();
		}
	}

	MonoAssemblyName* pAssemblyName = mono_assembly_get_name(pAssembly);
	string assemblyPath = mono_assembly_name_get_name(pAssemblyName);
	
	m_loadedLibraries.emplace_back(new CMonoLibrary(pAssembly, assemblyPath, this), assemblyPath);
	return m_loadedLibraries.back().pLibrary.get();
}

MonoAssembly* CMonoDomain::LoadMonoAssembly(const char* path, FILE* pFile, char** pImageDataOut, mono_byte** pDebugDataOut, bool bRefOnly)
{
	size_t imageSize = gEnv->pCryPak->FGetSize(pFile);
	*pImageDataOut = new char[imageSize];
	gEnv->pCryPak->FRead(*pImageDataOut, imageSize, pFile);

	MonoImageOpenStatus status = MONO_IMAGE_ERROR_ERRNO;

	// Load the assembly image, note the third argument being true.
	// We have to instruct mono to make a copy of imageData, otherwise they try to run 'free' on it later on.
	MonoImage* pAssemblyImage = mono_image_open_from_data_with_name(*pImageDataOut, imageSize, true, &status, bRefOnly, path);

	// Make sure to load references this image requires before loading the assembly itself
	for (int i = 0; i < mono_image_get_table_rows(pAssemblyImage, MONO_TABLE_ASSEMBLYREF); ++i) 
	{
		mono_assembly_load_reference(pAssemblyImage, i);
	}

	gEnv->pLog->LogWithType(IMiniLog::eMessage, "[Mono] Load Library: %s", path);

	// load debug information
#ifndef _RELEASE
	string sDebugDatabasePath = path;
	sDebugDatabasePath.append(".mdb");

	FILE* pDebugFile = gEnv->pCryPak->FOpen(sDebugDatabasePath, "rb", ICryPak::FLAGS_PATH_REAL);
	if (pDebugFile != nullptr)
	{
		size_t debugDatabaseSize = gEnv->pCryPak->FGetSize(pDebugFile);
		*pDebugDataOut = new mono_byte[debugDatabaseSize];
		gEnv->pCryPak->FRead(*pDebugDataOut, debugDatabaseSize, pDebugFile);

		mono_debug_open_image_from_memory(pAssemblyImage, *pDebugDataOut, debugDatabaseSize);

		gEnv->pCryPak->FClose(pDebugFile);
	}
#endif
	
	return mono_assembly_load_from_full(pAssemblyImage, path, &status, bRefOnly);
}