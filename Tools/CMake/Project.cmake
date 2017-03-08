# Include this Project.cmake from any game project cmake file to reference CRYENGINE
# Ex. include(${CRYENGINE_DIR}/Tools/CMake/Project.cmake)

cmake_minimum_required (VERSION 3.6.2)

# C/C++ languages required.
enable_language(C)
enable_language(CXX)
# Force C++11 support requirement
set (CMAKE_CXX_STANDARD 11)

#Validations of the Input parameters
if (NOT DEFINED CRYENGINE_DIR)
	set(CRYENGINE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

if (NOT DEFINED OUTPUT_DIRECTORY)
	set(OUTPUT_DIRECTORY "${CRYENGINE_DIR}")
endif()

set( TOOLS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR} )

#Fix slashes on paths
string(REPLACE "\\" "/" CRYENGINE_DIR ${CRYENGINE_DIR})
string(REPLACE "\\" "/" OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})
string(REPLACE "\\" "/" TOOLS_CMAKE_DIR ${TOOLS_CMAKE_DIR})

# Activate Visual Studio Solution Folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(OPTION_PCH "Enable Precompiled headers" ON)

if (WIN32)  # Either Win32 or Win64
	#Force Windows Toolchain
	set(WINDOWS TRUE)
	include(${CMAKE_CURRENT_LIST_DIR}/toolchain/windows/WindowsPC-MSVC.cmake)
endif(WIN32)

# Include Common CMake macros used by CRYENGINE cmake files
set(global_defines)
set(global_includes)
set(global_links)
include(${CMAKE_CURRENT_LIST_DIR}/CommonMacros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CommonOptions.cmake)

# Enables 3 Default Build Configurations (Debug/Profile/Release)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug Profile)
set(CMAKE_CONFIGURATION_TYPES Debug Profile Release)
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} CACHE STRING "Build Configurations" FORCE)

option(PROJECT_BUILD_CRYENGINE "Include CRYENGINE Modules in the generated Solution" OFF)
option(PROJECT_INCLUDE_CRYENGINE "Include CRYENGINE Modules in the Visual Studio generated Solution" OFF)
option(PROJECT_INCLUDE_CryAction "Include CryAction Module of CRYENGINE Modules in the Visual Studio generated Solution" ON)

if (PROJECT_INCLUDE_CRYENGINE)
	set( OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG})
	add_subdirectory( ${CRYENGINE_DIR} CE_BIN)
else()
	set(VS_FOLDER_PREFIX "CRYENGINE/")
	add_subdirectory( ${CRYENGINE_DIR}/Code/CryEngine/CryCommon CE_BIN_CryCommon )
	if (PROJECT_INCLUDE_CryAction)
		add_subdirectory( ${CRYENGINE_DIR}/Code/CryEngine/CryAction CE_BIN_CryAction )
	endif()
endif()

include_directories(${PROJECT_NAME} PRIVATE ${CRYENGINE_DIR}/Code/CryEngine/CryCommon)
if (PROJECT_INCLUDE_CryAction)
	include_directories(${PROJECT_NAME} PRIVATE ${CRYENGINE_DIR}/Code/CryEngine/CryAction)
endif()

foreach( define ${global_defines} )
	add_definitions(-D${define})
endforeach()
if(global_includes)
	include_directories(${global_includes})
endif()
if(global_links)
	link_directories(${global_links})
endif()