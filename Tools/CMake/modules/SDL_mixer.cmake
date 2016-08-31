set(SDL_MIXER_DIR "${SDK_DIR}/Audio/SDL_mixer")

if(WIN64)
	set(PLATFORM_DIR x64)
elseif(WIN32)
	set(PLATFORM_DIR x86)
endif()

if(WIN32)
	add_library(SDL_mixer SHARED IMPORTED GLOBAL)
	set_target_properties(SDL_mixer PROPERTIES IMPORTED_LOCATION "${SDL_MIXER_DIR}/lib/${PLATFORM_DIR}/SDL2_mixer.dll")
	set_target_properties(SDL_mixer PROPERTIES IMPORTED_IMPLIB "${SDL_MIXER_DIR}/lib/${PLATFORM_DIR}/SDL2_mixer.lib")
	set_target_properties(SDL_mixer PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SDL_MIXER_DIR}/include")


	deploy_runtime_files(${SDL_MIXER_DIR}/lib/${PLATFORM_DIR}/*.dll)
elseif(LINUX)
	message(STATUS "Linux binaries not available for SDL_mixer")
endif()

