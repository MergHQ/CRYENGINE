cmake_minimum_required(VERSION 3.6.2)

set(TOOLS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
include(${TOOLS_CMAKE_DIR}/Configure.cmake)

# Add custom project with just listing of cmake files
add_subdirectory(${TOOLS_CMAKE_DIR})

# Order currently matters for Engine, Games and Launchers
# 1. CryEngine
include ("${TOOLS_CMAKE_DIR}/BuildEngine.cmake")

# 2. Games
add_subdirectories_glob("Code/Game*")

# 3. Launchers
include ("${TOOLS_CMAKE_DIR}/BuildLaunchers.cmake")

# 3. Launchers
include ("${TOOLS_CMAKE_DIR}/BuildPlugins.cmake")

# Shaders custom project
add_subdirectory(Engine/Shaders)

if (OPTION_CRYMONO)
	add_subdirectory(Code/CryManaged/CryMonoBridge)
	add_subdirectory(Code/CryManaged/CESharp)
endif()

# Sandbox Editor
if (OPTION_SANDBOX AND WIN64)
	MESSAGE(STATUS "Include Sandbox Editor")
	include ("${TOOLS_CMAKE_DIR}/BuildSandbox.cmake")
endif()

if(WIN64 AND EXISTS "Code/Tools/ShaderCacheGen/ShaderCacheGen")
	option(OPTION_SHADERCACHEGEN "Build the shader cache generator." OFF)
endif()

if (OPTION_SHADERCACHEGEN)
	add_subdirectory(Code/Tools/ShaderCacheGen/ShaderCacheGen)
endif()

if (OPTION_RC AND EXISTS "Code/Tools/rc")
	include(ExternalProject)
	ExternalProject_Add(RC
		SOURCE_DIR "Code/Tools/rc"
		BUILD_COMMAND "${CMAKE_COMMAND}" --build "." --config $<$<CONFIG:Profile>:Release>$<$<NOT:$<CONFIG:Profile>>:$<CONFIG>>
		INSTALL_COMMAND echo "Skipping install"
	)
endif()

set(CMAKE_INSTALL_MESSAGE LAZY)
install(FILES ${TOOLS_CMAKE_DIR}/modules/CryCommonConfig.cmake DESTINATION share/cmake)
install(FILES ${TOOLS_CMAKE_DIR}/modules/CryActionConfig.cmake DESTINATION share/cmake)

copy_binary_files_to_target()
