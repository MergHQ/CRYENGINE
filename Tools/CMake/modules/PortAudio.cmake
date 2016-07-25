if(WIN64)
	add_library(libsndfile SHARED IMPORTED GLOBAL)
	set_target_properties(libsndfile PROPERTIES 
		IMPORTED_LOCATION "${SDK_DIR}/Audio/libsndfile/lib/win64/libsndfile-1.dll"
		IMPORTED_IMPLIB "${SDK_DIR}/Audio/libsndfile/lib/win64/libsndfile-1.lib")


	add_library(PortAudio SHARED IMPORTED GLOBAL)
	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(PortAudio PROPERTIES 
		IMPORTED_LOCATION_RELEASE "${SDK_DIR}/Audio/portaudio/lib/win64/release/portaudio_x64.dll"
		IMPORTED_IMPLIB_RELEASE "${SDK_DIR}/Audio/portaudio/lib/win64/release/portaudio_x64.lib")

	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS PROFILE)
	set_target_properties(PortAudio PROPERTIES
		IMPORTED_LOCATION_PROFILE "${SDK_DIR}/Audio/portaudio/lib/win64/profile/portaudio_x64.dll"
		IMPORTED_IMPLIB_PROFILE "${SDK_DIR}/Audio/portaudio/lib/win64/release/portaudio_x64.lib")

	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
	set_target_properties(PortAudio PROPERTIES 
		IMPORTED_LOCATION_DEBUG "${SDK_DIR}/Audio/portaudio/lib/win64/debug/portaudio_x64.dll"
		IMPORTED_IMPLIB_DEBUG "${SDK_DIR}/Audio/portaudio/lib/win64/debug/portaudio_x64.lib")
elseif(WIN32)
	add_library(libsndfile SHARED IMPORTED GLOBAL)
	set_target_properties(libsndfile PROPERTIES 
		IMPORTED_LOCATION "${SDK_DIR}/Audio/libsndfile/lib/win32/libsndfile-1.dll"
		IMPORTED_IMPLIB "${SDK_DIR}/Audio/libsndfile/lib/win32/libsndfile-1.lib")


	add_library(PortAudio SHARED IMPORTED GLOBAL)
	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(PortAudio PROPERTIES 
		IMPORTED_LOCATION_RELEASE "${SDK_DIR}/Audio/portaudio/lib/win32/release/portaudio_x86.dll"
		IMPORTED_IMPLIB_RELEASE "${SDK_DIR}/Audio/portaudio/lib/win32/release/portaudio_x86.lib")

	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS PROFILE)
	set_target_properties(PortAudio PROPERTIES
		IMPORTED_LOCATION_PROFILE "${SDK_DIR}/Audio/portaudio/lib/win32/profile/portaudio_x86.dll"
		IMPORTED_IMPLIB_PROFILE "${SDK_DIR}/Audio/portaudio/lib/win32/release/portaudio_x86.lib")

	set_property(TARGET PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
	set_target_properties(PortAudio PROPERTIES 
		IMPORTED_LOCATION_DEBUG "${SDK_DIR}/Audio/portaudio/lib/win32/debug/portaudio_x86.dll"
		IMPORTED_IMPLIB_DEBUG "${SDK_DIR}/Audio/portaudio/lib/win32/debug/portaudio_x86.lib")
endif()
if(WIN32 OR WIN64)
	set_target_properties(libsndfile PROPERTIES 
		INTERFACE_INCLUDE_DIRECTORIES ${SDK_DIR}/Audio/libsndfile/include)

	set_target_properties(PortAudio PROPERTIES 
		INTERFACE_LINK_LIBRARIES libsndfile
		INTERFACE_INCLUDE_DIRECTORIES ${SDK_DIR}/Audio/PortAudio/inc)
endif()
