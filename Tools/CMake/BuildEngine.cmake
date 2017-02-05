
#options

#Renderer modules
if(NOT (ORBIS OR ANDROID))
	OPTION(RENDERER_DX11 "Renderer for DirectX 11" ON)
endif()

if (WIN32 OR WIN64)
	if (NOT MSVC_VERSION EQUAL 1900)
		message(STATUS "MSVC 14.0 is not being used - CryRenderD3D12 cannot be built.")
	else()
		OPTION(RENDERER_DX12 "Renderer for DirectX 12" ON)
	endif()
endif()

if(LINUX OR ANDROID)
	set(RENDERER_OPENGL ON)
elseif (NOT (DURANGO OR ORBIS))
	OPTION(RENDERER_OPENGL "Renderer for OpenGL" ON)
endif()

if(ORBIS)
	OPTION(RENDERER_GNM "Use GNM renderer for Orbis" ON)
	if(NOT RENDERER_GNM)
		SET(RENDERER_DX11 ON)
	endif()
endif()

#Audio modules
if (ANDROID)
	# SDL_mixer is only supported audio on these platforms
	set(AUDIO_SDL_MIXER ON)
elseif (NOT (WIN32 OR WIN64 OR LINUX))
	# Wwise is only supported audio on these platforms
	set(AUDIO_WWISE ON)
else()
	if (EXISTS ${SDK_DIR}/Audio/fmod)
		option(AUDIO_FMOD "FMOD Studio support" ON)
		include(${TOOLS_CMAKE_DIR}/modules/fmod.cmake)
	endif()
	if (EXISTS ${SDK_DIR}/Audio/portaudio AND EXISTS ${SDK_DIR}/Audio/libsndfile AND (WIN32 OR WIN64))
		option(AUDIO_PORTAUDIO "PortAudio support" ON)
		include(${TOOLS_CMAKE_DIR}/modules/PortAudio.cmake)
	endif()
	if(WIN32)
		option(AUDIO_SDL_MIXER "SDL_mixer support" ON)
	endif()
	if (EXISTS ${SDK_DIR}/Audio/wwise)
		option(AUDIO_WWISE "Wwise support" ON)
	endif()
endif()

if(WIN32 OR WIN64)
	option(AUDIO_HRTF "HRTF support " ON)
endif()

#Physics modules
option(PHYSICS_CRYPHYSICS "Enable" ON)
if (WIN64)
	option(PHYSICS_PHYSX "Enable" OFF)
endif()
if (PHYSICS_PHYSX)
	include(${TOOLS_CMAKE_DIR}/modules/PhysX.cmake)
endif()

#rc
if(WIN32)
	if (NOT METADATA_COMPANY)
		set(METADATA_COMPANY "Crytek GmbH")
	endif()
	set(METADATA_COMPANY ${METADATA_COMPANY} CACHE STRING "Company name for executable metadata")

	if (NOT METADATA_COPYRIGHT)
		string(TIMESTAMP year "%Y")
		set(METADATA_COPYRIGHT "(C) ${year} ${METADATA_COMPANY}")
	endif()
	set(METADATA_COPYRIGHT ${METADATA_COPYRIGHT} CACHE STRING "Copyright string for executable metadata")	

	if (NOT VERSION)
		set(VERSION "1.0.0.0")
	endif()
	set(METADATA_VERSION ${VERSION} CACHE STRING "Version number for executable metadata" FORCE)
	string(REPLACE . , METADATA_VERSION_COMMA ${METADATA_VERSION})
endif(WIN32)


if (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
	set(MSVC_LIB_PREFIX vc140)
elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
	set(MSVC_LIB_PREFIX vc120)
elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
	set(MSVC_LIB_PREFIX "vc110")
else()
	set(MSVC_LIB_PREFIX "")
endif()


include_directories("Code/CryEngine/CryCommon")
include_directories("Code/CryEngine/CryCommon/3rdParty")
include_directories("${SDK_DIR}/boost")

if (WIN32)
	link_libraries (Ntdll)
endif (WIN32)

if (OPTION_SCALEFORMHELPER AND NOT (OPTION_ENGINE OR OPTION_SHADERCACHEGEN))
	add_subdirectory ("Code/CryEngine/CrySystem/Scaleform")
endif()

if (OPTION_ENGINE OR OPTION_SHADERCACHEGEN)
	add_subdirectory ("Code/CryEngine/CrySystem")
	add_subdirectory ("Code/CryEngine/CryCommon")
	add_subdirectory ("Code/CryEngine/RenderDll/XRenderD3D9")
endif()

#engine
if (OPTION_ENGINE)
	add_subdirectory ("Code/CryEngine/Cry3DEngine")
	add_subdirectory ("Code/CryEngine/CryAction")
	add_subdirectory ("Code/CryEngine/CryAISystem")
	add_subdirectory ("Code/CryEngine/CryAnimation")
	add_subdirectory ("Code/CryEngine/CryAudioSystem/Common")
	add_subdirectory ("Code/CryEngine/CryAudioSystem")
	add_subdirectory ("Code/CryEngine/CryDynamicResponseSystem")
	add_subdirectory ("Code/CryEngine/CryEntitySystem")
	add_subdirectory ("Code/CryEngine/CryFont")
	add_subdirectory ("Code/CryEngine/CryInput")
	add_subdirectory ("Code/CryEngine/CryMovie")
	add_subdirectory ("Code/CryEngine/CryNetwork")
	add_subdirectory ("Code/CryEngine/CrySchematyc")
	add_subdirectory ("Code/CryEngine/CryScriptSystem")
	add_subdirectory ("Code/CryEngine/CryFlowGraph")


	if (WIN32)
		add_subdirectory ("Code/CryEngine/CryLiveCreate")
	endif (WIN32)

	#physics
	if (PHYSICS_CRYPHYSICS)
		add_subdirectory ("Code/CryEngine/CryPhysics")
	endif()
	if (PHYSICS_PHYSX)
		add_subdirectory ("Code/CryEngine/CryPhysicsSystem/CryPhysX")
	endif()

	#audio
	if (AUDIO_FMOD)
		add_subdirectory ("Code/CryEngine/CryAudioSystem/implementations/CryAudioImplFmod")
	endif (AUDIO_FMOD)
	if (AUDIO_PORTAUDIO)
		add_subdirectory ("Code/CryEngine/CryAudioSystem/implementations/CryAudioImplPortAudio")
	endif (AUDIO_PORTAUDIO)
	if (AUDIO_SDL_MIXER)
		add_subdirectory ("Code/CryEngine/CryAudioSystem/implementations/CryAudioImplSDLMixer")
		add_subdirectory ("Code/Libs/flac")
		add_subdirectory ("Code/Libs/libmikmod")
		add_subdirectory ("Code/Libs/libmodplug")
		add_subdirectory ("Code/Libs/libogg")
		add_subdirectory ("Code/Libs/libvorbis")
		add_subdirectory ("Code/Libs/SDL_mixer")
		add_subdirectory ("Code/Libs/smpeg")
	endif (AUDIO_SDL_MIXER)
	if (AUDIO_WWISE)
		add_subdirectory ("Code/CryEngine/CryAudioSystem/implementations/CryAudioImplWwise")
	endif (AUDIO_WWISE)

	#libs
	add_subdirectory ("Code/Libs/bigdigits")
	if (WIN32)
		add_subdirectory ("Code/Libs/curl")
	endif ()
	add_subdirectory ("Code/Libs/freetype")
	add_subdirectory ("Code/Libs/lua")
endif()

if (OPTION_SCALEFORMHELPER OR OPTION_ENGINE OR OPTION_SHADERCACHEGEN)
	add_subdirectory ("Code/Libs/zlib")
	add_subdirectory ("Code/Libs/expat")
endif()

if (OPTION_ENGINE OR OPTION_SHADERCACHEGEN)
	add_subdirectory ("Code/Libs/tomcrypt")
	add_subdirectory ("Code/Libs/md5")
	add_subdirectory ("Code/Libs/lz4")
	add_subdirectory ("Code/Libs/jsmn")
	add_subdirectory ("Code/Libs/lzma")
	add_subdirectory ("Code/Libs/lzss")
endif()