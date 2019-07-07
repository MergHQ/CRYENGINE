// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <jni.h>
#include <android/log.h>

namespace Cry
{
	namespace JNI
	{
		//! Check if JNI is available in this DLL.
		//! Use to check for cases where JNI_OnLoad was potentially not called yet.
		//! E.g. initialization of the global data sector prior the DLL JNI_OnLoad entry.
		bool JNI_IsAvailable();

		//! Java VM
		JNIEnv* JNI_GetEnv(void);
		void JNI_AttachCurrentThread();
		void JNI_DetachCurrentThread();

		//! Java target: android.content.pm.ApplicationInfo.nativeLibraryDir
		const char* JNI_GetSharedLibDirectory();

		//! Java target: android.os.Environment.GetExternalStorageDirectory()
		const char* JNI_GetExternalStorageDirectory();

		//! Java target: android.content.Context.getFilesDir()
		const char* JNI_GetFilesDirectory();

		//! Java target: android.content.Context.getPackageName()
		const char* JNI_GetPackageName();

		//! Java target: java.lang.System.loadLibrary(string)
		//! Note: Do not provide "lib" prefix or ".so" extension
		//! Example: JNI_LoadLibrary("CrySystem")
		bool JNI_LoadLibrary(const char* libName);
	}  // namespace JNI
}  // namespace Cry
