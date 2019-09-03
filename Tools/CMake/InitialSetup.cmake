
# CRYENGINE_DIR is expected to be set by the starting CMakeLists.txt
if (NOT DEFINED CRYENGINE_DIR)
    message(FATAL_ERROR "CRYENGINE_DIR is not set. Please set it before including InitialSetup.cmake")
endif()

# The target in the TARGET signature of add_custom_command() must exist and must be defined in the current directory.
# This is a workaround for the add_custom_command for gmock breaking with Ninja build for certain platforms.
cmake_policy(SET CMP0040 OLD)

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Debug Profile)

# Turn on the ability to create folders to organize projects (.vcxproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcxproj and ZERO_CHECK.vcxproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set the PROJECT_DIR to the source path used to start CMake
if(NOT DEFINED PROJECT_DIR)
	set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
endif()

# Fix slashes on paths
file(TO_CMAKE_PATH "${CRYENGINE_DIR}" CRYENGINE_DIR)
file(TO_CMAKE_PATH "${PROJECT_DIR}" PROJECT_DIR)
file(TO_CMAKE_PATH "${TOOLS_CMAKE_DIR}" TOOLS_CMAKE_DIR)

set(TOOLS_CMAKE_DIR "${CRYENGINE_DIR}/Tools/CMake")
set(CMAKE_MODULE_PATH "${TOOLS_CMAKE_DIR}/modules")

if(NOT DEFINED SDK_DIR)
	set(SDK_DIR "${CRYENGINE_DIR}/Code/SDKs")
endif()

message(STATUS "CMAKE_GENERATOR = ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "OPTION_PROFILE = ${OPTION_PROFILE}")
message(STATUS "OPTION_PCH = ${OPTION_PCH}")
message(STATUS "CRYENGINE_DIR = ${CRYENGINE_DIR}")
message(STATUS "SDK_DIR = ${SDK_DIR}")
message(STATUS "PROJECT_DIR = ${PROJECT_DIR}")
message(STATUS "TOOLS_CMAKE_DIR = ${TOOLS_CMAKE_DIR}")

# Bootstrap support
if(EXISTS "${TOOLS_CMAKE_DIR}/Bootstrap.cmake")
	include("${TOOLS_CMAKE_DIR}/Bootstrap.cmake")
	if(OPTION_AUTO_BOOTSTRAP)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "bootstrap.dat")
	endif()
elseif(EXISTS "${TOOLS_CMAKE_DIR}/DownloadSDKs.cmake")
	include("${TOOLS_CMAKE_DIR}/DownloadSDKs.cmake")
endif()

# Including the Toolchain file, as it sets important variables.
if(DEFINED CMAKE_TOOLCHAIN_FILE)
	include(${CMAKE_TOOLCHAIN_FILE})
elseif(WIN32)
	include("${TOOLS_CMAKE_DIR}/toolchain/windows/WindowsPC-MSVC.cmake")
endif()

if (NOT DEFINED BUILD_PLATFORM)
	# For now, we expect BUILD_PLATFORM to have been set via a Toolchain file.
	message(FATAL_ERROR "BUILD_PLATFORM not defined. Please always supply one of the CRYENGINE toolchain files.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CYGWIN)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()
