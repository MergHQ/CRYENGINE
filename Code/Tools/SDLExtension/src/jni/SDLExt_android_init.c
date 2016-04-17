#ifdef __ANDROID__

#include <jni.h>

extern void SDLExt_Android_Init(JNIEnv* env, jclass cls, jobject mgr);

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeInitExt(JNIEnv* env, jclass cls, jobject mgr)
{
    SDLExt_Android_Init(env, cls, mgr);
}

#endif
