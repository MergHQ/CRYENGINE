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

# 3. Plugins
include ("${TOOLS_CMAKE_DIR}/BuildPlugins.cmake")

# 4. Launchers
include ("${TOOLS_CMAKE_DIR}/BuildLaunchers.cmake")


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

# Run Unit Test
if (OPTION_ENGINE AND (WIN32 OR WIN64))
	add_custom_target(run_unit_tests)
	set_target_properties(run_unit_tests PROPERTIES EXCLUDE_FROM_ALL TRUE)
	set_target_properties(run_unit_tests PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)
	set_property(TARGET run_unit_tests PROPERTY FOLDER "_TEST_")
	get_property(WindowsLauncherExe TARGET WindowsLauncher PROPERTY OUTPUT_NAME )
	#message("WindowsLauncherExe = ${WindowsLauncherExe}")
	file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/run_unit_tests.vcxproj.user" 
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>
	<Project ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Profile|x64'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>		
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Profile|Win32'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>		
			<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">
				<LocalDebuggerCommand>${OUTPUT_DIRECTORY}/${WindowsLauncherExe}.exe</LocalDebuggerCommand>
				<LocalDebuggerWorkingDirectory>${OUTPUT_DIRECTORY}</LocalDebuggerWorkingDirectory>
				<LocalDebuggerCommandArguments>-run_unit_tests</LocalDebuggerCommandArguments>
				<DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
			</PropertyGroup>
		</Project>")
endif()

if(WIN64 AND EXISTS "Code/Tools/ShaderCacheGen/ShaderCacheGen")
	option(OPTION_SHADERCACHEGEN "Build the shader cache generator." OFF)
endif()

if (OPTION_SHADERCACHEGEN)
	add_subdirectory(Code/Tools/ShaderCacheGen/ShaderCacheGen)
endif()

if (OPTION_PAKTOOLS AND EXISTS "Code/Tools/PakEncrypt")
	add_subdirectory(Code/Tools/PakEncrypt)
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
