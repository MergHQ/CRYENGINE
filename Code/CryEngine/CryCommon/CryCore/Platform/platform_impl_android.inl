// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AndroidJNI.inl"

void CryFindEngineRootFolder(unsigned int engineRootPathSize, char* szEngineRootPath)
{
	// Hack! Android currently does not support a directory layout, there is an explicit search in main for GameSDK/GameData.pak
	// and the executable folder is not related to the engine or game folder. - 18/03/2016
	cry_strcpy(szEngineRootPath, engineRootPathSize, CryGetProjectStoragePath());
}


#include <cstdio>
// Returns path to CRYENGINE and Crytek provided 3rd Party shared libraries 
const char* CryGetSharedLibraryStoragePath()
{
	return Cry::JNI::JNI_GetSharedLibDirectory();
}

// Get path to user folder
const char* CryGetUserStoragePath()
{
	return Cry::JNI::JNI_GetFilesDirectory();
}

// Get path to project root. i.e. assets are stored in a sub folder here
const char* CryGetProjectStoragePath()
{
	// For now assume the assets are directly on the external storage. Not even in a CE sub folder.
	// They should move to a folder that is persistent to the application but hidden away to the user
	// i.e. Cry::JNI::JNI_GetFilesDirectory();
	return Cry::JNI::JNI_GetExternalStorageDirectory();
}

#include <dlfcn.h>
// Returns a handle to the launcher
void* CryGetLauncherModuleHandle()
{
#if CRY_PLATFORM_ANDROID
	// On Android dlopen(NULL, RTLD_LAZY) does not provide the handle to the launcher dll file but to some other dll.
	return CryLoadLibrary("libAndroidLauncher.so");
#else
	return ::dlopen(NULL, RTLD_LAZY):
#endif
	
	// On Windows
	//GetModuleHandle(0)
}
