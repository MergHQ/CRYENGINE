cmake_minimum_required (VERSION 3.6.2)

#Validations of the Input parameters
if (NOT DEFINED CRYENGINE_DIR)
	message( FATAL_ERROR "CRYENGINE_DIR variable must be defined and point to a valid path to CryEngine root folder." )
endif()

option(PROJECT_INCLUDE_CryAction "Include CryAction Module of CRYENGINE Modules in the Visual Studio generated Solution" OFF)

# Include Common CMake macros used by CRYENGINE cmake files
include(${CRYENGINE_DIR}/Tools/CMake/Project.cmake)

project (Plugin VERSION 1.0 LANGUAGES CXX)

set (SourceGroup_PluginMain
	"StdAfx.cpp"
	"StdAfx.h"
	"Plugin.cpp"
	"Plugin.h"
)
source_group("Main" FILES ${SourceGroup_PluginMain})

set (SOURCE
	${SourceGroup_PluginMain}
)


add_library(${PROJECT_NAME} MODULE ${SOURCE})
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE Ntdll)
endif(WIN32)

#Enable Precompiled Headers
USE_MSVC_PRECOMPILED_HEADER(${PROJECT_NAME} "StdAfx.h" "StdAfx.cpp")

# Set Visual Studio solution project name
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Plugins")
