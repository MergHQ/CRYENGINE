cmake_minimum_required(VERSION 3.5)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug Profile)

set (CryEngine_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CryEngine_DIR}/Tools/CMake/modules")

# C/C++ languages required.
enable_language(C)
enable_language(CXX)
# Force C++11 support requirement
set (CMAKE_CXX_STANDARD 11)

if (DURANGO OR ORBIS OR ANDROID OR LINUX)
	unset(WIN32)
	unset(WIN64)
endif ()

if (WIN32)  # Either Win32 or Win64
	set(WINDOWS TRUE)
	include(Tools/CMake/toolchain/windows/WindowsPC-MSVC.cmake)
endif(WIN32)

if(NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")
	set(valid_configs Debug Profile Release)
	list(FIND valid_configs "${CMAKE_BUILD_TYPE}" config_index)
	if(${config_index} EQUAL -1)
		message(SEND_ERROR "Build type \"${CMAKE_BUILD_TYPE}\" is not supported, set CMAKE_BUILD_TYPE to one of ${valid_configs}")
	endif()
endif()

include(${CryEngine_DIR}/Tools/CMake/CopyFilesToBin.cmake)

set(METADATA_PROJECT_NAME "CryEngine" CACHE STRING "Name of the solution project")
project(${METADATA_PROJECT_NAME}_CMake_${BUILD_PLATFORM} CXX C)

set(MODULES CACHE INTERNAL "Modules for monolithic builds" FORCE)
set(GAME_MODULES CACHE INTERNAL "Game Modules for monolithic builds" FORCE)

if (DURANGO OR ORBIS OR ANDROID OR LINUX)
	# WIN32 Should be unset  again after project( line to work correctly
	unset(WIN32)
	unset(WIN64)
endif ()

set(game_folder CACHE INTERNAL "Game folder used for resource files on Windows" FORCE)

if(LINUX32 OR LINUX64)
	set(LINUX TRUE)
endif()

# Define Options
option(OPTION_PROFILE "Enable Profiling" ON)
option(OPTION_UNITY_BUILD "Enable Unity Build" ON)
if(WIN32 OR WIN64)
	option(OPTION_ENABLE_BROFILER "Enable Brofiler profiler support" ON)
endif()

if(WIN64 AND EXISTS "${CryEngine_DIR}/Code/Sandbox")
	option(OPTION_SANDBOX "Enable Sandbox" ON)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
		set(OPTION_SANDBOX OFF)
	endif()
	if(OPTION_SANDBOX)
		# Sandbox cannot be built in release mode
		set(CMAKE_CONFIGURATION_TYPES Debug Profile CACHE STRING "Reset the configurations to what we need" FORCE)
	endif()
endif()

option(OPTION_RC "Include RC in the build" OFF)
option(OPTION_PCH "Enable Precompiled Headers" ON)

#Plugins
option(PLUGIN_FPSPLUGIN "Frames per second sample plugin" OFF)

option(PLUGIN_SCHEMATYC "Enable Schematyc" ON)

if(WIN32 OR WIN64)
	option(PLUGIN_VR_OCULUS "Oculus support" ON)
	option(PLUGIN_VR_OSVR "OSVR support" ON)
	option(PLUGIN_VR_OPENVR "OpenVR support" ON)
	option(OPTION_CRYMONO "C# support" OFF)
endif()

if (WIN32 OR WIN64)
	option(OPTION_ENABLE_CRASHRPT "Enable CrashRpt crash reporting library" ON)
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

# SDK Directory
set(SDK_DIR ${CryEngine_DIR}/Code/SDKs)
set(CRY_LIBS_DIR ${CryEngine_DIR}/Code/Libs)
set(CRY_EXTENSIONS_DIR ${CryEngine_DIR}/Code/CryExtensions)

set(WINSDK_SDK_DIR "${SDK_DIR}/Microsoft Windows SDK")
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

	# Make sure the output directory exists
	file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})
endif (OUTPUT_DIRECTORY)

# Bootstrap support
if(EXISTS ${CryEngine_DIR}/Tools/CMake/Bootstrap.cmake)
	include(${CryEngine_DIR}/Tools/CMake/Bootstrap.cmake)
	if(OPTION_AUTO_BOOTSTRAP)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "bootstrap.dat")
	endif()
endif()

include(${CryEngine_DIR}/Tools/CMake/ConfigureChecks.cmake)
include(${CryEngine_DIR}/Tools/CMake/CommonMacros.cmake)

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

if(ORBIS AND NOT RENDERER_GNM)
	SET(RENDERER_DX11 ON)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/SDL2.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/SDL2_mixer.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/Boost.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Tools/CMake/modules/ncurses.cmake)

if (OPTION_SANDBOX AND WIN64)
	# Find Qt before including any plugin subdirectories
	if (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
		set(QT_DIR ${CryEngine_DIR}/Code/SDKs/Qt/5.6/msvc2015_64/Qt)
	elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
		set(QT_DIR ${CryEngine_DIR}/Code/SDKs/Qt/5.6/msvc2013_64)
	elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
		set(QT_DIR ${CryEngine_DIR}/Code/SDKs/Qt/5.6/msvc2012_64)
	endif()
	set(Qt5_DIR ${QT_DIR})

	find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED PATHS "${QT_DIR}")
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER  "AUTOGEN")
	set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER  "AUTOMOC")
endif()

# CryEngine
include ("${CryEngine_DIR}/Tools/CMake/BuildEngine.cmake")

# Shaders custom project
add_subdirectory(Engine/Shaders)

if (WIN32)
	add_subdirectory(Code/CryEngine/CryLiveCreate)
	# CryExtensions
	#add_subdirectory(Code/CryExtensions/GameStream)
endif (WIN32)

# CryExtensions
add_subdirectory(Code/CryExtensions)

if (PLUGIN_SCHEMATYC)
	add_subdirectory(Code/CryPlugins/CrySensorSystem/Module)
endif()

# Example plugin
if (PLUGIN_FPS)
	add_subdirectory(Code/CryPlugins/CryFramesPerSecond/Module)
endif()
# VR plugins
if (PLUGIN_VR_OCULUS)
	add_subdirectory(Code/Libs/oculus)
	add_subdirectory(Code/CryPlugins/VR/CryOculusVR/Module)
endif()
if (PLUGIN_VR_OPENVR)	
	add_subdirectory(Code/CryPlugins/VR/CryOpenVR/Module)
endif()	
if (PLUGIN_VR_OSVR)	
	add_subdirectory(Code/CryPlugins/VR/CryOSVR/Module)
endif()



add_custom_target(CE DEPENDS CryPhysics CrySystem CryAction CryRenderD3D11
		CryFont CryNetwork CryMovie CryInput CryAISystem CryAnimation Cry3DEngine
		CryScriptSystem CryEntitySystem CryAudioSystem CryAudioImplSDLMixer
		CryDynamicResponseSystem)


# Game
add_subdirectories_glob("Code/Game*")

if (OPTION_CRYMONO)
	add_subdirectory(Code/CryManaged/CryMonoBridge)
	add_subdirectory(Code/CryManaged/CESharp)
endif()

# Sandbox Editor
if (OPTION_SANDBOX AND WIN64)
	MESSAGE(STATUS "Include Sandbox Editor")
	include ("${CryEngine_DIR}/Tools/CMake/BuildSandbox.cmake")
endif()

# UQS: Optional plugin; option PLUGIN_CRYUQS to enable/disable it resides in its own sub directory
add_subdirectory(Code/CryPlugins/CryUQS)

if(WIN64 AND EXISTS "${CryEngine_DIR}/Code/Tools/ShaderCacheGen/ShaderCacheGen")
	option(OPTION_SHADERCACHEGEN "Build the shader cache generator." OFF)
endif()

if (OPTION_SHADERCACHEGEN)
	add_subdirectory(Code/Tools/ShaderCacheGen/ShaderCacheGen)
endif()

if (OPTION_RC AND EXISTS "${CryEngine_DIR}/Code/Tools/rc")
	include(ExternalProject)
	ExternalProject_Add(RC
		SOURCE_DIR "${CryEngine_DIR}/Code/Tools/rc"
		INSTALL_COMMAND echo "Skipping install"
	)
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
install(FILES Tools/CMake/modules/CryCommonConfig.cmake DESTINATION share/cmake)
install(FILES Tools/CMake/modules/CryActionConfig.cmake DESTINATION share/cmake)

copy_binary_files_to_target()
