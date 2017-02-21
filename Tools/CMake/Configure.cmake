#Configures CRYENGINE CMake settings

cmake_minimum_required(VERSION 3.6.2)

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug Profile)

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT DEFINED CRYENGINE_DIR)
	set (CRYENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

set( TOOLS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR} )

#Fix slashes on paths
string(REPLACE "\\" "/" CRYENGINE_DIR ${CRYENGINE_DIR})
string(REPLACE "\\" "/" TOOLS_CMAKE_DIR ${TOOLS_CMAKE_DIR})
if (OUTPUT_DIRECTORY)
	string(REPLACE "\\" "/" OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
endif()

list(APPEND CMAKE_MODULE_PATH "${TOOLS_CMAKE_DIR}/modules")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} CACHE INTERNAL "CMake module path" FORCE)

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
	include(${TOOLS_CMAKE_DIR}/toolchain/windows/WindowsPC-MSVC.cmake)
endif(WIN32)

if(NOT ${CMAKE_GENERATOR} MATCHES "Visual Studio")
	set(valid_configs Debug Profile Release)
	list(FIND valid_configs "${CMAKE_BUILD_TYPE}" config_index)
	if(${config_index} EQUAL -1)
		message(SEND_ERROR "Build type \"${CMAKE_BUILD_TYPE}\" is not supported, set CMAKE_BUILD_TYPE to one of ${valid_configs}")
	endif()
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}_release)
endif()

set(METADATA_PROJECT_NAME "CryEngine" CACHE STRING "Name of the solution project")
project(${METADATA_PROJECT_NAME}_CMake_${BUILD_PLATFORM} CXX C)

# Prefix all Visual Studio solution folder names with this string
set( VS_FOLDER_PREFIX "CRYENGINE/" )

set(MODULES CACHE INTERNAL "List of engine modules being built" FORCE)
set(GAME_MODULES CACHE INTERNAL "List of game modules being built" FORCE)

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
option(OPTION_ENGINE "Enable CRYENGINE" ON)

option(OPTION_PROFILE "Enable Profiling" ON)
option(OPTION_UNITY_BUILD "Enable Unity Build" ON)
if(WIN32 OR WIN64)
	option(OPTION_ENABLE_BROFILER "Enable Brofiler profiler support" ON)
endif()

if(WIN64 AND EXISTS "${CRYENGINE_DIR}/Code/Sandbox/EditorQt")
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

if(WIN32 OR WIN64)
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
MESSAGE(STATUS "MSVC = ${MSVC}")
MESSAGE(STATUS "CRYENGINE_DIR = ${CRYENGINE_DIR}")

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
set(SDK_DIR ${CRYENGINE_DIR}/Code/SDKs)
set(CRY_LIBS_DIR ${CRYENGINE_DIR}/Code/Libs)
set(CRY_EXTENSIONS_DIR ${CRYENGINE_DIR}/Code/CryExtensions)

set(WINSDK_SDK_DIR "${SDK_DIR}/Microsoft Windows SDK")
set(WINSDK_SDK_LIB_DIR "${WINSDK_SDK_DIR}/V8.0/Lib/Win8/um")
set(WINSDK_SDK_INCLUDE_DIR "${WINSDK_SDK_DIR}/V8.0/Include/um")

# Must be included after SDK_DIR definition
include(${TOOLS_CMAKE_DIR}/CopyFilesToBin.cmake)

# custom defines
set(global_defines "CRYENGINE_DEFINE")

if(NOT ANDROID AND NOT ORBIS)
	option(OPTION_SCALEFORMHELPER "Use Scaleform Helper" ON)
endif()

if(OPTION_STATIC_LINKING)
	# Enable static libraries
	MESSAGE(STATUS "Use Static Linking (.lib/.a)" )
	set(BUILD_SHARED_LIBS FALSE)
	if(OPTION_SANDBOX)
		message(STATUS "Disabling Sandbox - requires dynamic linking")
		set(OPTION_SANDBOX OFF)
	endif()
else()
	# Enable dynamic libraries
	MESSAGE(STATUS "Use Dynamic Linking (.dll/.so)" )
	set(BUILD_SHARED_LIBS TRUE)
endif()

if (OUTPUT_DIRECTORY)
	message(STATUS "OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}")
	set(EXECUTABLE_OUTPUT_PATH "${OUTPUT_DIRECTORY}")

	# Make sure the output directory exists
	file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})
endif (OUTPUT_DIRECTORY)

# Bootstrap support
if(EXISTS ${TOOLS_CMAKE_DIR}/Bootstrap.cmake)
	include(${TOOLS_CMAKE_DIR}/Bootstrap.cmake)
	if(OPTION_AUTO_BOOTSTRAP)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "bootstrap.dat")
	endif()
endif()

include(${TOOLS_CMAKE_DIR}/ConfigureChecks.cmake)
include(${TOOLS_CMAKE_DIR}/CommonMacros.cmake)

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

include(${TOOLS_CMAKE_DIR}/modules/SDL2.cmake)
include(${TOOLS_CMAKE_DIR}/modules/Boost.cmake)
include(${TOOLS_CMAKE_DIR}/modules/ncurses.cmake)

if (OPTION_SANDBOX AND WIN64)
	# Find Qt before including any plugin subdirectories
	if (MSVC_VERSION GREATER 1900) # Visual Studio > 2015
		set(QT_DIR ${SDK_DIR}/Qt/5.6/msvc2015_64/Qt)
	elseif (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
		set(QT_DIR ${SDK_DIR}/Qt/5.6/msvc2015_64/Qt)
	elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
		set(QT_DIR ${SDK_DIR}/Qt/5.6/msvc2013_64)
	elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
		set(QT_DIR ${SDK_DIR}/Qt/5.6/msvc2012_64)
	endif()
	set(Qt5_DIR ${QT_DIR})

	find_package(Qt5 COMPONENTS Core Gui OpenGL Widgets REQUIRED PATHS "${QT_DIR}")
	set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER  "${VS_FOLDER_PREFIX}/Sandbox/AUTOGEN")
endif()

