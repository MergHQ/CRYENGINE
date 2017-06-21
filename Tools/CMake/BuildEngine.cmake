
#options

option(PLUGIN_SCHEMATYC "Enables compilation of the Schematyc plugin" ON)

option(OPTION_PAKTOOLS "Build .pak encryption tools" OFF)
option(OPTION_RC "Include RC in the build" OFF)

if (WIN32 OR WIN64)
	option(OPTION_ENABLE_CRASHRPT "Enable CrashRpt crash reporting library" ON)
endif()

if(NOT ANDROID AND NOT ORBIS)
	option(OPTION_SCALEFORMHELPER "Use Scaleform Helper" ON)
else()
	set(OPTION_SCALEFORMHELPER ON)
endif()

#Plugins
option(PLUGIN_FPSPLUGIN "Frames per second sample plugin" OFF)
if(WIN32 OR WIN64)
	option(PLUGIN_USERANALYTICS "Enable User Analytics" ON)
	option(PLUGIN_VR_OCULUS "Oculus support" ON)
	option(PLUGIN_VR_OSVR "OSVR support" ON)
	option(PLUGIN_VR_OPENVR "OpenVR support" ON)
	option(OPTION_CRYMONO "C# support" OFF)
	
	if (OPTION_CRYMONO)
		option(OPTION_CRYMONO_SWIG "Expose C++ API to C# with SWIG" ON)
	endif()
	
	option(OPTION_PHYSDBGR "Include standalone physics debugger in the build" OFF)
endif()

option(OPTION_UNSIGNED_PAKS_IN_RELEASE "Allow unsigned PAK files to be used for release builds" ON)

if(WIN64 AND EXISTS "${CRYENGINE_DIR}/Code/Sandbox/EditorQt")
	option(OPTION_SANDBOX "Enable Sandbox" ON)
	if (EXISTS "${SDK_DIR}/SubstanceEngines")
		option(OPTION_SANDBOX_SUBSTANCE "Enable Sandbox Substance Integration" ON)
	endif()
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		set(OPTION_SANDBOX OFF)
	endif()
	if(OPTION_SANDBOX)
		# Sandbox cannot be built in release mode
		set(CMAKE_CONFIGURATION_TYPES Debug Profile CACHE STRING "Reset the configurations to what we need" FORCE)
	else()
		if(OPTION_SANDBOX_SUBSTANCE)
			set(OPTION_SANDBOX_SUBSTANCE OFF)
		endif()
	endif()
endif()

if(WIN32 OR WIN64)
	option(OPTION_ENABLE_BROFILER "Enable Brofiler profiler support" ON)
endif()

#Renderer modules
if(NOT (ORBIS OR ANDROID OR LINUX))
	OPTION(RENDERER_DX11 "Renderer for DirectX 11" ON)
endif()

if (WIN32 OR WIN64)
	if (MSVC_VERSION LESS 1900)
		message(STATUS "MSVC 14.0 or above is required to build CryRenderD3D12.")
	else()
		OPTION(RENDERER_DX12 "Renderer for DirectX 12" ON)
	endif()
endif()

# Disable opengl renderer for now
set(RENDERER_OPENGL OFF)

if(ORBIS)
	OPTION(RENDERER_GNM "Use GNM renderer for Orbis" ON)
	if(NOT RENDERER_GNM)
		SET(RENDERER_DX11 ON)
	endif()
endif()

if(WIN64)
  OPTION(RENDERER_VULKAN "Renderer for Vulkan API" ON)
endif()

#Audio modules
if (ANDROID)
	# SDL_mixer is only supported audio on these platforms
	set(AUDIO_SDL_MIXER ON)
elseif (NOT (WIN32 OR WIN64 OR LINUX))
	# Wwise is only supported audio on these platforms
	set(AUDIO_WWISE ON)
else()
	if (EXISTS "${SDK_DIR}/Audio/fmod")
		option(AUDIO_FMOD "FMOD Studio support" ON)
		include(${TOOLS_CMAKE_DIR}/modules/fmod.cmake)
	endif()
	if (EXISTS "${SDK_DIR}/Audio/portaudio" AND EXISTS "${SDK_DIR}/Audio/libsndfile" AND (WIN32 OR WIN64))
		option(AUDIO_PORTAUDIO "PortAudio support" ON)
		include(${TOOLS_CMAKE_DIR}/modules/PortAudio.cmake)
	endif()
	if(WIN32)
		option(AUDIO_SDL_MIXER "SDL_mixer support" ON)
	endif()
	if (EXISTS "${SDK_DIR}/Audio/wwise")
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
	include("${TOOLS_CMAKE_DIR}/modules/PhysX.cmake")
endif()

include_directories("${CRYENGINE_DIR}/Code/CryEngine/CryCommon")
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CryCommon/3rdParty")
include_directories("${SDK_DIR}/boost")
include_directories("${CRYENGINE_DIR}/Code/CryEngine/CrySchematyc/Core/Interface")

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
	
	# Shaders custom project
	add_subdirectory(Engine/Shaders)
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
	if (WIN32 OR WIN64)
		add_subdirectory ("Code/Libs/png16")
	endif ()
endif()

if (OPTION_ENGINE OR OPTION_SHADERCACHEGEN OR OPTION_PAKTOOLS)
	add_subdirectory ("Code/Libs/tomcrypt")
endif()

if (OPTION_ENGINE OR OPTION_SHADERCACHEGEN)
	add_subdirectory ("Code/Libs/md5")
	add_subdirectory ("Code/Libs/lz4")
	add_subdirectory ("Code/Libs/jsmn")
	add_subdirectory ("Code/Libs/lzma")
	add_subdirectory ("Code/Libs/lzss")
	add_subdirectory ("Code/Libs/tiff")
endif()
