set(FMOD_DIR "${SDK_DIR}/Audio/fmod")

if(WINDOWS)
	add_library(fmod SHARED IMPORTED GLOBAL)
	set(FMOD_DLL "${FMOD_DIR}/windows/api/lowlevel/lib/fmod64.dll")
	set_target_properties(fmod PROPERTIES IMPORTED_LOCATION "${FMOD_DLL}")
	set_target_properties(fmod PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR}/windows/api/lowlevel/lib/fmod64_vc.lib")
	set_target_properties(fmod PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/windows/api/lowlevel/inc")

	add_library(fmodstudio SHARED IMPORTED GLOBAL)
	set(FMODSTUDIO_DLL "${FMOD_DIR}/windows/api/studio/lib/fmodstudio64.dll")
	set_target_properties(fmodstudio PROPERTIES IMPORTED_LOCATION "${FMODSTUDIO_DLL}")
	set_target_properties(fmodstudio PROPERTIES IMPORTED_IMPLIB "${FMOD_DIR}/windows/api/studio/lib/fmodstudio64_vc.lib")
	set_target_properties(fmodstudio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/windows/api/studio/inc")

elseif(LINUX)
	add_library(fmod SHARED IMPORTED GLOBAL)
	set(FMOD_DLL "${FMOD_DIR}/linux/api/lowlevel/lib/x86_64/libfmod.so")
	set_target_properties(fmod PROPERTIES IMPORTED_LOCATION "${FMOD_DLL}")
	set_target_properties(fmod PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/linux/api/lowlevel/inc")

	add_library(fmodstudio SHARED IMPORTED GLOBAL)
	set(FMODSTUDIO_DLL "${FMOD_DIR}/linux/api/studio/lib/x86_64/libfmodstudio.so")
	set_target_properties(fmodstudio PROPERTIES IMPORTED_LOCATION "${FMODSTUDIO_DLL}")
	set_target_properties(fmodstudio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/linux/api/studio/inc")

elseif(ANDROID)
	add_library(fmod SHARED IMPORTED GLOBAL)
	set(FMOD_DLL "${FMOD_DIR}/android/api/lowlevel/lib/${CMAKE_ANDROID_ARCH_ABI}/libfmod.so")
	set_target_properties(fmod PROPERTIES IMPORTED_LOCATION "${FMOD_DLL}")
	set_target_properties(fmod PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/android/api/lowlevel/inc")

	add_library(fmodstudio SHARED IMPORTED GLOBAL)
	set(FMODSTUDIO_DLL "${FMOD_DIR}/android/api/studio/lib/${CMAKE_ANDROID_ARCH_ABI}/libfmodstudio.so")
	set_target_properties(fmodstudio PROPERTIES IMPORTED_LOCATION "${FMODSTUDIO_DLL}")
	set_target_properties(fmodstudio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FMOD_DIR}/android/api/studio/inc")

endif()

if(AUDIO_FMOD AND FMOD_DLL)
	set_target_properties(fmod PROPERTIES INTERFACE_COMPILE_DEFINITIONS FMOD_FOR_$<UPPER_CASE:$<CONFIG>>)
	deploy_runtime_files("${FMOD_DLL}")
	deploy_runtime_files("${FMODSTUDIO_DLL}")
endif()
