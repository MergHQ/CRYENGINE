start_sources()

sources_platform(ALL)
add_sources("CryAudioImpl_uber_0.cpp"
	SOURCE_GROUP "Header Files"
		"Common.h"
		"CVars.h"
		"Event.h"
		"EventInstance.h"
		"File.h"
		"GlobalData.h"
		"Impl.h"
		"Listener.h"
		"Object.h"
		"resource.h"
		"VolumeParameter.h"
		"VolumeParameterAdvanced.h"
		"VolumeState.h"
	SOURCE_GROUP "Source Files"
		"Common.cpp"
		"CryAudioImpl.cpp"
		"CVars.cpp"
		"Event.cpp"
		"EventInstance.cpp"
		"Impl.cpp"
		"Listener.cpp"
		"Object.cpp"
		"VolumeParameter.cpp"
		"VolumeParameterAdvanced.cpp"
		"VolumeState.cpp"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"stdafx.cpp"
		"stdafx.h"
)

end_sources()

if(AUDIO_SDL_MIXER)
	CryEngineModule(CryAudioImplSDLMixer FORCE_SHARED_WIN PCH "stdafx.cpp" SOLUTION_FOLDER "CryEngine/Audio/Implementations")
	target_link_libraries(${THIS_PROJECT} PRIVATE sdl_mixer)
	target_compile_definitions(${THIS_PROJECT} PRIVATE $<$<NOT:$<CONFIG:Release>>:CRY_AUDIO_IMPL_SDLMIXER_USE_DEBUG_CODE>)
	target_include_directories(${THIS_PROJECT} PRIVATE ../../Common)
else()
	CryFileContainer(CryAudioImplSDLMixer SOLUTION_FOLDER "CryEngine/Audio/Implementations")
endif()
if (ORBIS OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-format
		-Wno-multichar
		-Wno-parentheses
		-Wno-unknown-pragmas
		-Wno-unused-function
		-Wno-unused-private-field
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-format
		-Wno-maybe-uninitialized
		-Wno-misleading-indentation
		-Wno-multichar
		-Wno-parentheses
		-Wno-sign-compare
		-Wno-strict-aliasing
		-Wno-unknown-pragmas
		-Wno-unused-but-set-variable
		-Wno-unused-function
	)
endif()
