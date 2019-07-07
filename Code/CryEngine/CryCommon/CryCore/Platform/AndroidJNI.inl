// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <CryCore/Platform/AndroidJNI.h>

namespace Cry
{
	namespace JNI
	{
		namespace detail
		{
			const int g_requiredJNIVersion = JNI_VERSION_1_6;
			JavaVM* g_pJavaVM = nullptr;
		}  // namespace detail

		bool JNI_HandleException()
		{
			JNIEnv* env = JNI_GetEnv();
			if (!env->ExceptionCheck())
			{
				return false;
			}

			// Get the exception and clear as no
			// JNI calls can be made while an exception exists.
			jthrowable e = env->ExceptionOccurred();

			env->ExceptionDescribe();
			env->ExceptionClear();

			jclass clazz = env->GetObjectClass(e);
			jmethodID getMessage = env->GetMethodID(clazz, "getMessage", "()Ljava/lang/String;");
			jstring message = (jstring)env->CallObjectMethod(e, getMessage);
			const char* exceptionString = env->GetStringUTFChars(message, NULL);

			CryLogAlways(exceptionString);

			env->ReleaseStringUTFChars(message, exceptionString);
			env->DeleteLocalRef(message);
			env->DeleteLocalRef(clazz);
			env->DeleteLocalRef(e);

			return true;
		}

		JNIEnv* JNI_GetEnv(void)
		{
			CRY_ASSERT(detail::g_pJavaVM);

			thread_local JNIEnv* tl_JNIEnv = nullptr;  // the environment is only valid for the thread
			if (!tl_JNIEnv)
			{
				int getEnvStat = detail::g_pJavaVM->GetEnv((void**)&tl_JNIEnv, detail::g_requiredJNIVersion);

				// Ensure that the thread loading the library is attached to the JavaVM
				if (getEnvStat == JNI_EDETACHED)
				{
					if (detail::g_pJavaVM->AttachCurrentThread(&tl_JNIEnv, nullptr) != 0)
					{
						__android_log_print(ANDROID_LOG_ERROR, "Cry", "JNI: Failed to attach to thread.");
					}
				}
				else if (getEnvStat == JNI_EVERSION)
				{
					__android_log_print(ANDROID_LOG_ERROR, "Cry", "JNI: Version not supported.");
				}
			}
			return tl_JNIEnv;
		}

		jobject JNI_GetGlobalContext()
		{
			JNIEnv* env = JNI_GetEnv();
			jclass activityThread = env->FindClass("android/app/ActivityThread");
			jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
			jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);

			jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
			jobject context = env->CallObjectMethod(at, getApplication);

			env->DeleteLocalRef(at);
			env->DeleteLocalRef(activityThread);

			// Commented out on purpose! - The caller has to free the context
			// env->DeleteLocalRef(context);

			return context;
		}

		void JNI_AttachCurrentThread()
		{
			JNI_GetEnv();
		}

		void JNI_DetachCurrentThread()
		{
			JNIEnv* pJNIEnv = nullptr;
			int getEnvStat = detail::g_pJavaVM->GetEnv((void**)&pJNIEnv, detail::g_requiredJNIVersion);

			// Ensure that the thread was attached to the JavaVM
			if (getEnvStat == JNI_OK)
			{
				detail::g_pJavaVM->DetachCurrentThread();
			}
		}

		const char* JNI_GetSharedLibDirectory()
		{
			static char s_shareLibDirectory[MAX_PATH] = {0};

			if (s_shareLibDirectory[0] == '\0')
			{
				JNIEnv* env = JNI_GetEnv();
				jobject context = JNI_GetGlobalContext();
				jclass Context_class = env->FindClass("android/content/Context");
				jmethodID midGetApplicationInfo = env->GetMethodID(Context_class, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
				jobject appInfo = env->CallObjectMethod(context, midGetApplicationInfo);

				jfieldID fieldID = env->GetFieldID(env->GetObjectClass(appInfo), "nativeLibraryDir", "Ljava/lang/String;");
				jstring nativeLibraryDir = (jstring)env->GetObjectField(appInfo, fieldID);
				const char* str = env->GetStringUTFChars(nativeLibraryDir, NULL);
				cry_strcpy(s_shareLibDirectory, str);

				env->ReleaseStringUTFChars(nativeLibraryDir, str);
				env->DeleteLocalRef(appInfo);
				env->DeleteLocalRef(Context_class);
				env->DeleteLocalRef(context);
			}

			return s_shareLibDirectory;
		}

		const char* JNI_GetExternalStorageDirectory()
		{
			static char s_externalStorageDirectory[MAX_PATH] = {0};
			if (s_externalStorageDirectory[0] == '\0')
			{
				JNIEnv* env = JNI_GetEnv();
				jclass cls = env->FindClass("android/os/Environment");
				jmethodID mid = env->GetStaticMethodID(cls, "getExternalStorageDirectory", "()Ljava/io/File;");
				jobject fileObject = env->CallStaticObjectMethod(cls, mid);

				mid = env->GetMethodID(env->GetObjectClass(fileObject), "getAbsolutePath", "()Ljava/lang/String;");				
				jstring pathString = (jstring)env->CallObjectMethod(fileObject, mid);
				const char* path = env->GetStringUTFChars(pathString, NULL);
				cry_strcpy(s_externalStorageDirectory, path);

				env->ReleaseStringUTFChars(pathString, path);
				env->DeleteLocalRef(fileObject);
				env->DeleteLocalRef(cls);
			}
			return s_externalStorageDirectory;
		}

		const char* JNI_GetFilesDirectory()
		{
			static char s_filesDirectory[MAX_PATH] = {0};

			if (s_filesDirectory[0] == '\0')
			{
				JNIEnv* env = JNI_GetEnv();
				jobject context = JNI_GetGlobalContext();
				jclass Context_class = env->FindClass("android/content/Context");
				jmethodID mid = env->GetMethodID(Context_class, "getFilesDir", "()Ljava/io/File;");
				jobject fileObject = env->CallObjectMethod(context, mid);

				mid = env->GetMethodID(env->GetObjectClass(fileObject), "getAbsolutePath", "()Ljava/lang/String;");
				jstring pathString = (jstring)env->CallObjectMethod(fileObject, mid);
				const char* path = env->GetStringUTFChars(pathString, NULL);
				cry_strcpy(s_filesDirectory, path);

				env->ReleaseStringUTFChars(pathString, path);
				env->DeleteLocalRef(fileObject);
				env->DeleteLocalRef(Context_class);
				env->DeleteLocalRef(context);
			}
			return s_filesDirectory;
		}

		const char* JNI_GetPackageName()
		{
			static char s_AndroidPackageName[MAX_PATH] = {0};
			if (s_AndroidPackageName[0] == '\0')
			{
				JNIEnv* env = JNI_GetEnv();
				jobject context = JNI_GetGlobalContext();

				jmethodID mid = env->GetMethodID(env->GetObjectClass(context), "getPackageName", "()Ljava/lang/String;");
				jstring nameString = (jstring)env->CallObjectMethod(context, mid);
				const char* name = env->GetStringUTFChars(nameString, NULL);
				cry_strcpy(s_AndroidPackageName, name);

				env->ReleaseStringUTFChars(nameString, name);
				env->DeleteLocalRef(context);
			}

			return s_AndroidPackageName;
		}

		bool JNI_IsAvailable()
		{
			return detail::g_pJavaVM ? true : false;
		}

		bool JNI_LoadLibrary(const char* libName)
		{
			__android_log_print(ANDROID_LOG_INFO, "Cry", "JNI: Attempt to load library: %s", libName);

			JNIEnv* env = JNI_GetEnv();
			jclass system_class = env->FindClass("java/lang/System");
			jmethodID midInitialize = env->GetStaticMethodID(system_class, "loadLibrary", "(Ljava/lang/String;)V");
			jstring jstrBuf = env->NewStringUTF(libName);

			// Clear exceptions before loading library
			env->ExceptionClear();
			env->CallStaticVoidMethod(system_class, midInitialize, jstrBuf);

			env->DeleteLocalRef(jstrBuf);
			env->DeleteLocalRef(system_class);

			return !JNI_HandleException();
		}
	}  // namespace JNI
}  // namespace Cry

// Called by JavaVM when library is loaded via a Java side LoadLibrary call
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	Cry::JNI::detail::g_pJavaVM = vm;
	return Cry::JNI::detail::g_requiredJNIVersion;
}

// Called by JavaVM when library is unloaded via a Java side UnloadLibrary call
JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved)
{
}
