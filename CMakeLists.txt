cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Tools/CMake/modules")

enable_language(C)
enable_language(CXX)

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug Profile)

if (DURANGO OR ORBIS OR ANDROID)
	unset(WIN32)
	unset(WIN64)	
endif ()

if(WIN32)
	set(CMAKE_SYSTEM_VERSION 10.0.10586.0)
	include(${CMAKE_SOURCE_DIR}/Tools/CMake/MSVC-Windows.cmake)
endif()

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if(NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")
	set(valid_configs Debug Profile Release)
	list(FIND valid_configs "${CMAKE_BUILD_TYPE}" config_index)
	if(${config_index} EQUAL -1)
		message(SEND_ERROR "Build type \"${CMAKE_BUILD_TYPE}\" is not supported, set CMAKE_BUILD_TYPE to one of ${valid_configs}")
	endif()
endif()

include(${CMAKE_SOURCE_DIR}/Tools/CMake/CopyFilesToBin.cmake)

#set(ANDROID TRUE)

if(ANDROID)
	unset(WIN32)
	unset(WIN64)
endif(ANDROID)

project(CryEngine_CMake_${BUILD_PLATFORM} CXX C)

set(MODULES CACHE INTERNAL "Modules for monolithic builds" FORCE)
set(GAME_MODULES CACHE INTERNAL "Game Modules for monolithic builds" FORCE)

if (DURANGO OR ORBIS OR ANDROID)
	# WIN32 Should be unset  again after project( line to work correctly
	unset(WIN32)
	unset(WIN64)
endif ()

if(WIN32)
	set(WINDOWS TRUE)
endif()

if(LINUX32 OR LINUX64)
	set(LINUX TRUE)
endif()

# Define Options
option(OPTION_PROFILE "Enable Profiling" ON)
option(OPTION_UNITY_BUILD "Enable Unity Build" ON)
if(WIN32 OR WIN64)
	option(OPTION_ENABLE_BROFILER "Enable Brofiler profiler support" ON)
endif()

if(WIN64 AND EXISTS "${CMAKE_SOURCE_DIR}/Code/Sandbox")
	set(OPTION_SANDBOX "No" CACHE STRING "Enable Sandbox")
	set_property(CACHE OPTION_SANDBOX PROPERTY STRINGS "No" "Qt" "Legacy")
endif()

option(OPTION_PCH "Enable Precompiled Headers" OFF)

#Extensions

#option(EXTENSION_CRYDESCRIPTOR "Enable CryDescriptor" ON)
#option(EXTENSION_CRYDRSCOMMON "Enable CryDRSCommonElements" ON)
option(EXTENSION_CRYLOBBY "Enable CryLobby" ON)
#option(EXTENSION_SCHEMATYC "Enable Schematyc" ON)
#option(EXTENSION_CRYLINK "Enable CryLink" ON)

#Plugins
option(PLUGIN_FPSPLUGIN "Frames per second sample plugin" OFF)
option(PLUGIN_OCULUSPLUGIN "Oculus support" ON)
option(PLUGIN_OPENVRPLUGIN "OpenVR support" ON)
option(PLUGIN_OSVRPLUGIN "OSVR support" ON)

if(WIN32 OR WIN64)
	option(OPTION_CRYMONO "C# support" OFF)
endif()

# Print current project settings
MESSAGE(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
MESSAGE(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")
MESSAGE(STATUS "CMAKE_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
MESSAGE(STATUS "BUILD_PLATFORM = ${BUILD_PLATFORM}")
MESSAGE(STATUS "OPTION_PROFILE = ${OPTION_PROFILE}")
MESSAGE(STATUS "OPTION_PCH = ${OPTION_PCH}")

if (OPTION_PROFILE)
	set_property( DIRECTORY PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>:_PROFILE> )
else()
	set_property( DIRECTORY PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>:_RELEASE> )
endif()

if(OPTION_UNITY_BUILD)
	message(STATUS "UNITY BUILD Enabled")
endif()

if(ORBIS OR ANDROID)
	set(OPTION_STATIC_LINKING TRUE)
else()
	option(OPTION_STATIC_LINKING "Link all CryEngine modules as static libs to single exe" OFF)
endif()

if (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
	set(MSVC_LIB_PREFIX vc140)
	set(QT_DIR ${CMAKE_SOURCE_DIR}/Code/SDKs/Qt/5.6/msvc2015_64)
elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
	set(MSVC_LIB_PREFIX vc120)
	set(QT_DIR ${CMAKE_SOURCE_DIR}/Code/SDKs/Qt/5.6/msvc2013_64)
elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
	set(MSVC_LIB_PREFIX "vc110")
	set(QT_DIR ${CMAKE_SOURCE_DIR}/Code/SDKs/Qt/5.6/msvc2012_64)
else()
	set(MSVC_LIB_PREFIX "")
endif()
set(Qt5_DIR ${QT_DIR})

# SDK Directory
set(SDK_DIR ${CMAKE_SOURCE_DIR}/Code/SDKs)
set(CRY_LIBS_DIR ${CMAKE_SOURCE_DIR}/Code/Libs)
set(CRY_EXTENSIONS_DIR ${CMAKE_SOURCE_DIR}/Code/CryExtensions)
#set(DXSDK_DIR ${CMAKE_SOURCE_DIR}/Code/SDKs/DXSDK)
set(WINSDK_SDK_DIR "${CMAKE_SOURCE_DIR}/Code/SDKs/Microsoft Windows SDK")
set(WINSDK_SDK_LIB_DIR "${WINSDK_SDK_DIR}/V8.0/Lib/Win8/um")
set(WINSDK_SDK_INCLUDE_DIR "${WINSDK_SDK_DIR}/V8.0/Include/um")

# custom defines
set(global_defines "CRYENGINE_DEFINE")

if(OPTION_STATIC_LINKING)
	# Enable static libraries
	MESSAGE(STATUS "Use Static Linking (.lib)" )
	set(BUILD_SHARED_LIBS FALSE)
else()
	# Enable dynamic libraries
	MESSAGE(STATUS "Use Dynamic Linking (.dll)" )
	set(BUILD_SHARED_LIBS TRUE)
endif()

if (OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
	set(EXECUTABLE_OUTPUT_PATH "${OUTPUT_DIRECTORY}")
endif (OUTPUT_DIRECTORY)

# Bootstrap support
if(EXISTS ${CMAKE_SOURCE_DIR}/Tools/CMake/Bootstrap.cmake)
	include(${CMAKE_SOURCE_DIR}/Tools/CMake/Bootstrap.cmake)
	if(OPTION_AUTO_BOOTSTRAP)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "bootstrap.dat")
	endif()
endif()

include(${CMAKE_SOURCE_DIR}/Tools/CMake/ConfigureChecks.cmake)
include(${CMAKE_SOURCE_DIR}/Tools/CMake/CommonMacros.cmake)

include_directories( ${SDK_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/Code/CryEngine/CryCommon )

set( BOOST_DIR ${SDK_DIR}/boost )
include_directories( ${BOOST_DIR} )

if(WIN32)
	# Common Libriries linked to all targets
	set(COMMON_LIBS Ntdll User32 Advapi32 Ntdll Ws2_32)

	if (EXISTS ${SDK_DIR}/GPA)
		include_directories( ${SDK_DIR}/GPA/include )
		if (WIN64)
			link_directories( ${SDK_DIR}/GPA/lib64 )
		else()
			link_directories( ${SDK_DIR}/GPA/lib32 )
		endif()
		set(COMMON_LIBS ${COMMON_LIBS} jitprofiling libittnotify)
	endif()
else()
	# Common Libriries linked to all targets
	set(COMMON_LIBS )
endif()

# Serialization library
include_directories( ${CRY_LIBS_DIR}/yasli )
include_directories( ${SDK_DIR}/yasli )

# add global defines
foreach( current_define ${platform_defines} )
	add_definitions(-D${current_define})
endforeach()

if ((WIN32 OR WIN64) AND OPTION_ENABLE_BROFILER)
	add_definitions(-DUSE_BROFILER)
	include_directories( ${SDK_DIR}/Brofiler )
	link_directories( ${SDK_DIR}/Brofiler )
	if (WIN64)
		set(COMMON_LIBS ${COMMON_LIBS} ProfilerCore64)
	elseif(WIN32)
		set(COMMON_LIBS ${COMMON_LIBS} ProfilerCore32)
	endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if (OPTION_GAME_PROJECT STREQUAL "BenchmarkVR")
	add_subdirectory(Code/Game_Skyharbor/BenchmarkFramework)
	add_subdirectory(Code/Game_Skyharbor/GameDll)
	set_solution_folder("Projects" BenchmarkFramework)
	set_solution_folder("Projects" CryGameSkyharbor)
endif()

#Audio modules
if (NOT (WIN32 OR WIN64 OR LINUX))
	# WWise is only supported audio on these platforms
	set(AUDIO_WWISE ON)
else()
	if (EXISTS ${SDK_DIR}/Audio/fmod)
		include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/fmod.cmake)
		option(AUDIO_FMOD "FMod sound system support" ON)
	endif()
	if (EXISTS ${SDK_DIR}/Audio/portaudio AND EXISTS ${SDK_DIR}/Audio/libsndfile AND (WIN32 OR WIN64))
		include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/PortAudio.cmake)
		option(AUDIO_PORTAUDIO "PortAudio sound system support" ON)
	endif()
	option(AUDIO_SDL_MIXER "SDLMixer sound system support" ON)
	if (EXISTS ${SDK_DIR}/Audio/wwise)
		option(AUDIO_WWISE "WWise Sound System support" ON)
	endif()
endif()

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

if (EXISTS ${SDK_DIR}/Orbis AND EXISTS ${CMAKE_SOURCE_DIR/}Code/CryEngine/RenderDll/XRenderD3D9/GNM)
	if(ORBIS)
		#Force DX11 renderer
		#option(RENDERER_GNM "GNM renderer" ON)
	endif()
	if(WIN64)
		#option(RENDERER_GNM_SHADER_COMPILER "Shader compiler for Orbis" ON)
	endif()
endif()

if(ORBIS AND NOT RENDERER_GNM)
	SET(RENDERER_DX11 ON)
endif()


include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/FbxSdk.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/SDL2.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/SDL_mixer.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/Boost.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/ncurses.cmake)

# CryEngine
add_subdirectory(Code/CryEngine/Cry3DEngine)
add_subdirectory(Code/CryEngine/CryAction)
add_subdirectory(Code/CryEngine/CryAISystem)
add_subdirectory(Code/CryEngine/CryAnimation)
add_subdirectory(Code/CryEngine/CryDynamicResponseSystem)
add_subdirectory(Code/CryEngine/CryEntitySystem)
add_subdirectory(Code/CryEngine/CryFont)
add_subdirectory(Code/CryEngine/CryInput)
add_subdirectory(Code/CryEngine/CryMovie)
add_subdirectory(Code/CryEngine/CryNetwork)
add_subdirectory(Code/CryEngine/CryPhysics)
add_subdirectory(Code/CryEngine/RenderDll/XRenderD3D9)
add_subdirectory(Code/CryEngine/CryScriptSystem)
add_subdirectory(Code/CryEngine/CryAudioSystem)
add_subdirectory(Code/CryEngine/CryAudioSystem/Common)
add_subdirectory(Code/CryEngine/CryAudioSystem/implementations/CryAudioImplFmod)
add_subdirectory(Code/CryEngine/CryAudioSystem/implementations/CryAudioImplPortAudio)
add_subdirectory(Code/CryEngine/CryAudioSystem/implementations/CryAudioImplSDLMixer)
add_subdirectory(Code/CryEngine/CryAudioSystem/implementations/CryAudioImplWwise)
add_subdirectory(Code/CryEngine/CrySystem)

# CryCommon headers only project
add_subdirectory(Code/CryEngine/CryCommon)

# Shaders custom project
add_subdirectory(Engine/Shaders)

if (WIN32)
	add_subdirectory(Code/CryEngine/CryLiveCreate)
	# CryExtensions
	#add_subdirectory(Code/CryExtensions/GameStream)
endif (WIN32)

# CryExtensions
if (EXTENSION_CRYDESCRIPTOR)
	#add_subdirectory(Code/CryExtensions/CryDescriptor)
endif()	
if (EXTENSION_CRYDRSCOMMON)
	#add_subdirectory(Code/CryExtensions/CryDrsCommonElements/Module)
endif()

if (EXTENSION_CRYLOBBY)
	add_subdirectory(Code/CryExtensions/CryLobby/Module)
endif()
if (EXTENSION_SCHEMATYC)
	#add_subdirectory(Code/CryExtensions/CrySchematyc)
endif()
if (EXTENSION_CRYLINK)
	#add_subdirectory(Code/CryExtensions/CryLink/Module)
endif()




# Example plugin
if (PLUGIN_FPS)
	add_subdirectory(Code/CryPlugins/CryFramesPerSecond/Module)
endif()
# VR plugins
if (PLUGIN_OCULUS)
	add_subdirectory(Code/CryPlugins/CryOculusVR/Module)
endif()
if (PLUGIN_OPENVR)	
	add_subdirectory(Code/CryPlugins/CryOpenVR/Module)
endif()	
if (PLUGIN_OSVR)	
	add_subdirectory(Code/CryPlugins/CryOSVR/Module)
endif()

add_custom_target(CE DEPENDS CryPhysics CrySystem CryAction CryRenderD3D11
		CryFont CryNetwork CryMovie CryInput CryAISystem CryAnimation Cry3DEngine
		CryScriptSystem CryEntitySystem CryAudioSystem CryAudioImplSDLMixer
		CryDynamicResponseSystem)


# Game
option(OPTION_GAMESDK "Build GameSDK sample project" ON)
option(OPTION_GAMEZERO "Build GameZero sample project" OFF)
option(OPTION_GAMEMONO "Build GameMono sample project" OFF)

if (OPTION_GAMESDK) 
	add_subdirectory(Code/GameSDK/GameDll)
endif()

if (OPTION_GAMEZERO) 
	add_subdirectory(Code/GameZero/GameDll)
endif()

if (OPTION_GAMEMONO)
	if (NOT OPTION_CRYMONO)
		message(WARNING "Forcing C# support on for GameMono")
		set(OPTION_CRYMONO ON)
	endif()
	add_subdirectory(Code/GameMono/GameDll)
endif()

if (OPTION_CRYMONO)
	add_subdirectory(Code/CryManaged/CryMonoBridge)
	add_subdirectory(Code/CryManaged/CESharp)
endif()

# Launchers
if (DURANGO)
	add_subdirectory(Code/Launcher/DurangoLauncher)
elseif (ORBIS)
	add_subdirectory(Code/Launcher/OrbisLauncher)
elseif (ANDROID)
	add_subdirectory(Code/Launcher/AndroidLauncher)
elseif (WIN32)
	add_subdirectory(Code/Launcher/DedicatedLauncher)
	add_subdirectory(Code/Launcher/WindowsLauncher)
endif ()

#Libs
add_subdirectory(Code/Libs/freetype)
add_subdirectory(Code/Libs/expat)
add_subdirectory(Code/Libs/lua)
add_subdirectory(Code/Libs/zlib)
add_subdirectory(Code/Libs/lz4)	
add_subdirectory(Code/Libs/md5)
add_subdirectory(Code/Libs/tiff)
add_subdirectory(Code/Libs/lzma)
add_subdirectory(Code/Libs/lzss)
add_subdirectory(Code/Libs/bigdigits)
add_subdirectory(Code/Libs/strophe)
add_subdirectory(Code/Libs/yasli)
add_subdirectory(Code/Libs/tomcrypt)
add_subdirectory(Code/Libs/prt)
add_subdirectory(Code/Libs/python)
add_subdirectory(Code/Libs/jsmn)
add_subdirectory(Code/Libs/vco)
add_subdirectory(Code/Libs/oculus)

# Sandbox Editor
if (OPTION_SANDBOX AND WIN64)
	MESSAGE(STATUS "Include ${OPTION_SANDBOX} Sandbox Editor")
	find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED PATHS "${QT_DIR}")
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER  "AUTOGEN")
	set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER  "AUTOMOC")	

	if (OPTION_SANDBOX STREQUAL "Qt")
		set(EDITOR_DIR "${CMAKE_SOURCE_DIR}/Code/Sandbox/EditorQt" )
		add_subdirectory(Code/Sandbox/EditorQt)
		add_subdirectory(Code/Sandbox/Plugins/3DConnexionPlugin)	
		add_subdirectory(Code/Sandbox/Plugins/EditorConsole)
		add_subdirectory(Code/CryExtensions/CryLink/EditorPlugin)
		
		add_custom_target(GameAndTools DEPENDS ${GAME_TARGET} Sandbox)
	elseif(OPTION_SANDBOX STREQUAL "Legacy")
		set(EDITOR_DIR "${CMAKE_SOURCE_DIR}/Code/Sandbox/Editor" )
		add_subdirectory(Code/Sandbox/Editor)

		add_custom_target(GameAndTools DEPENDS ${GAME_TARGET} SandboxLegacy)
	endif()

	add_subdirectory(Code/Sandbox/Plugins/EditorCommon)

	add_subdirectory(Code/Sandbox/Plugins/CryDesigner)	
	add_subdirectory(Code/Sandbox/Plugins/EditorAudioControlsEditor)
	add_subdirectory(Code/Sandbox/Plugins/EditorAudioControlsEditor/common)
	if(AUDIO_FMOD)
		add_subdirectory(Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorFmod)
	endif()
	if(AUDIO_SDL_MIXER)
		add_subdirectory(Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorSDLMixer)
	endif()
	if (AUDIO_WWISE)
		add_subdirectory(Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorWwise)
	endif()	
	add_subdirectory(Code/Sandbox/Plugins/EditorAnimation)
	add_subdirectory(Code/Sandbox/Plugins/EditorDynamicResponseSystem)
	add_subdirectory(Code/Sandbox/Plugins/EditorEnvironment)
	add_subdirectory(Code/Sandbox/Plugins/EditorParticle)
	add_subdirectory(Code/Sandbox/Plugins/EditorTrackView)
	add_subdirectory(Code/Sandbox/Plugins/FbxPlugin)
	add_subdirectory(Code/Sandbox/Plugins/MeshImporter)
	add_subdirectory(Code/Sandbox/Plugins/PerforcePlugin)
endif()

if (WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/Code/Tools/ShaderCacheGen/ShaderCacheGen")
	add_subdirectory(Code/Tools/ShaderCacheGen/ShaderCacheGen EXCLUDE_FROM_ALL)
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
install(FILES Tools/CMake/modules/CryCommonConfig.cmake DESTINATION share/cmake)
install(FILES Tools/CMake/modules/CryActionConfig.cmake DESTINATION share/cmake)

copy_binary_files_to_target()
