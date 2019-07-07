// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

/*!
   CryLibrary

   Convenience-Macros which abstract the use of DLLs/shared libraries in a platform independent way.
   A short explanation of the different macros follows:

   CrySharedLibrarySupported:
    This macro can be used to test if the current active platform supports shared library calls. The default
    value is false. This gets redefined if a certain platform (Windows or Linux) is desired.

   CrySharedLibraryPrefix:
    The default prefix which will get prepended to library names in calls to CryLoadLibraryDefName
    (see below).

   CrySharedLibraryExtension:
    The default extension which will get appended to library names in calls to CryLoadLibraryDefName
    (see below).

   CryLoadLibrary(libName):
    Loads a shared library.

   CryLoadLibraryDefName(libName):
    Loads a shared library. The platform-specific default library prefix and extension are appended to the libName.
    This allows writing of somewhat platform-independent library loading code and is therefore the function
    which should be used most of the time, unless some special extensions are used (e.g. for plugins).

   CryGetProcAddress(libHandle, procName):
    Import function from the library presented by libHandle.

   CryGetModuleFileName(module, filename, size)
    Finds the file referred to by *module*, storing it in *filename*.
	If NULL is passed, it gets the path of the currently-running executable.
	
   CryFreeLibrary(libHandle):
    Unload the library presented by libHandle.

   HISTORY:
    03.03.2004 MarcoK
      - initial version
      - added to CryPlatform
 */

#include <stdio.h>
#include "CryPlatformDefines.h"

#if CRY_PLATFORM_WINDOWS || CRY_PLATFORM_DURANGO
	#include <CryCore/Platform/CryWindows.h>
	#if CRY_PLATFORM_WINDOWS
		#define CryLoadLibrary(libName) ::LoadLibraryA(libName)
	#elif CRY_PLATFORM_DURANGO
		#define CryLoadLibrary(libName) ::LoadLibraryExA(libName, 0, 0)
	#endif
	#define CryGetCurrentModule()                         ::GetModuleHandle(nullptr)
	#define CryGetModuleFileName(module, filename, size)  ::GetModuleFileName(module, filename, size)
	#define CrySharedLibrarySupported   true
	#define CrySharedLibraryPrefix      ""
	#define CrySharedLibraryExtension   ".dll"
	#define CryGetProcAddress(libHandle, procName) ::GetProcAddress((HMODULE)(libHandle), procName)
	#define CryFreeLibrary(libHandle)              ::FreeLibrary((HMODULE)(libHandle))
#elif CRY_PLATFORM_LINUX || CRY_PLATFORM_ANDROID || CRY_PLATFORM_APPLE
	#include <dlfcn.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <cstring>

	// for compatibility with code written for windows
	#define CrySharedLibrarySupported     true
	#define CrySharedLibraryPrefix        "lib"
	#if CRY_PLATFORM_APPLE
		#define CrySharedLibraryExtension ".dylib"
	#else
		#define CrySharedLibraryExtension ".so"
	#endif

	#define CryGetProcAddress(libHandle, procName) ::dlsym(libHandle, procName)
	#define CryFreeLibrary(libHandle)              ::dlclose(libHandle)
	#define CryGetCurrentModule()                  ::dlopen(NULL, RTLD_LAZY)
	#define HMODULE                                void*

static size_t CryGetModuleFileName(void* handle, char path[], size_t size)
{
	if(handle == nullptr)
	{
		readlink("/proc/self/exe", path, size);
		return strlen(path);
	}

	Dl_info info;

	::dladdr(handle, &info);
	if(info.dli_sname == NULL && info.dli_saddr == NULL)
		return 0;

	size_t len = strlen(info.dli_fname);
	std::memcpy(path, info.dli_fname, len);
	return len;
}

static const char* gEnvName("MODULE_PATH");

static const char* GetModulePath()
{
	return getenv(gEnvName);
}

static void SetModulePath(const char* pModulePath)
{
	setenv(gEnvName, pModulePath ? pModulePath : "", true);
}

static HMODULE CryLoadLibrary(const char* libName, bool bLazy = false, bool bInModulePath = true)
{
	if (strlen(libName) == 0)
	{
		return NULL;
	}

	char finalPath[_MAX_PATH] = {};
	CRY_ASSERT(strlen(libName) > CRY_ARRAY_COUNT(CrySharedLibraryPrefix));
	CRY_ASSERT(strlen(libName) > CRY_ARRAY_COUNT(CrySharedLibraryExtension));

	const char* filePre = cry_strncmp(libName, CrySharedLibraryPrefix) != 0 ? CrySharedLibraryPrefix : "";
	const char* fileExt = cry_strncmp(libName + strlen(libName) - (CRY_ARRAY_COUNT(CrySharedLibraryExtension) - 1), CrySharedLibraryExtension) != 0 ? CrySharedLibraryExtension : "";

#if CRY_PLATFORM_ANDROID
	// 1) Load dll via Java -> ensure JNI_OnLoad is called and all native exported functions are exposed to java
	// Need to be called first to ensure JNI_OnLoad() is called the first time we load the library
	if (Cry::JNI::JNI_IsAvailable())
	{
		// Call library via Java so we invoke JNI_OnLoad and load the library symbols into the global space
		// Note: Also call dlopen(libName) so that we can get the handle ... the library should have been loaded via the JNI_LoadLibrary() call already.
		char strippedLibName[128] = {};
		const int lenFilePre = strlen(filePre) == 0 ? CRY_ARRAY_COUNT(CrySharedLibraryPrefix) - 1 : 0;
		const int lenFileExt = strlen(fileExt) == 0 ? CRY_ARRAY_COUNT(CrySharedLibraryExtension) - 2 : 0;
		const char* strippedNameBegin = &libName[lenFilePre];
		cry_sprintf(strippedLibName, strlen(strippedNameBegin) - lenFileExt, "%s", strippedNameBegin);

		if (!Cry::JNI::JNI_LoadLibrary(strippedLibName))
		{
			return NULL;  // mimic dlopen() return for failed load
		}
	}

	// 2) We need the handle to the dll for dlsym() so we need to load it again

	// Since Android 7.0 (SDK 24) dlopen()  does not require the full path qualifier anymore
	HMODULE dllHandle = ::dlopen(libName, bLazy ? RTLD_LAZY : RTLD_NOW);
	if (dllHandle)
	{
		return dllHandle;
	}

	// 3) Pre-Android 7.0 (SDK 24)  dlopen() required a full path qualifier to the lib directory
	// Note: Only works if the JAVA Vm has been setup via a JNI_OnLoad() call in this calling library.
	const char* libPath = "";
	if (Cry::JNI::JNI_IsAvailable())
	{
		libPath = CryGetSharedLibraryStoragePath();
	}
#else
	const char* libPath = bInModulePath ? (GetModulePath() ? GetModulePath() : ".") : "";
#endif	

	cry_sprintf(finalPath, "%s%s%s%s%s", libPath, libPath ? "/" : "", filePre, libName, fileExt);

#if CRY_PLATFORM_LINUX
	return ::dlopen(finalPath, (bLazy ? RTLD_LAZY : RTLD_NOW) | RTLD_DEEPBIND);
#else
	return ::dlopen(finalPath, bLazy ? RTLD_LAZY : RTLD_NOW);
#endif
}
#else
	#define CrySharedLibrarySupported              false
	#define CrySharedLibraryPrefix                 ""
	#define CrySharedLibraryExtension              ""
	#define CryLoadLibrary(libName)                NULL
	#define CryGetProcAddress(libHandle, procName) NULL
	#define CryFreeLibrary(libHandle)
	#define GetModuleHandle(x)                     0
	#define CryGetCurrentModule()                  NULL
#endif

#define CryLibraryDefName(libName)               CrySharedLibraryPrefix libName CrySharedLibraryExtension
#define CryLoadLibraryDefName(libName)           CryLoadLibrary(CryLibraryDefName(libName))

// RAII helper to load a dynamic library and free it at the end of the scope.
class CCryLibrary
{
public:
	CCryLibrary(const char* szLibraryPath)
		: m_hModule(nullptr)
	{
		if (szLibraryPath != nullptr)
		{
			m_hModule = CryLoadLibrary(szLibraryPath);
		}
	}

	CCryLibrary(const CCryLibrary& other) = delete;

	CCryLibrary(CCryLibrary&& other)
		: m_hModule(std::move(other.m_hModule))
	{
		other.m_hModule = nullptr;
	}

	~CCryLibrary()
	{
		Free();
	}

	void Free()
	{
		if (m_hModule != nullptr)
		{
			CryFreeLibrary(m_hModule);
			m_hModule = nullptr;
		}
	}

	void ReleaseOwnership() { m_hModule = nullptr; }
	bool IsLoaded() const { return m_hModule != nullptr; }

	template<typename TProcedure>
	TProcedure GetProcedureAddress(const char* szName)
	{
		return (TProcedure)CryGetProcAddress(m_hModule, szName);
	}

	void Set(const char* szLibraryPath)
	{
		if (m_hModule != nullptr)
		{
			CryFreeLibrary(m_hModule);
			m_hModule = nullptr;
		}

		if (szLibraryPath != nullptr)
		{
			m_hModule = CryLoadLibrary(szLibraryPath);
		}
	}

	HMODULE m_hModule;
};
