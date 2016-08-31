function(add_prefix list_name prefix)
	set( ${list_name}_TMP )
	foreach(s ${${list_name}})
		list(APPEND ${list_name}_TMP "${prefix}${s}")
	endforeach()
	set( ${list_name} ${${list_name}_TMP} PARENT_SCOPE)
endfunction()

function(add_suffix list_name suffix)
	set( ${list_name}_TMP )
	foreach(s ${${list_name}})
		list(APPEND ${list_name}_TMP "${s}${suffix}")
	endforeach()
	set( ${list_name} ${${list_name}_TMP} PARENT_SCOPE)
endfunction()

function(make_library list_name path)
	set( ${list_name}_TMP )
	foreach(s ${${list_name}})
		if (WIN32 OR WIN64 OR DURANGO)
			list(APPEND ${list_name}_TMP "${path}${s}.lib")
		else()
			list(APPEND ${list_name}_TMP "${path}lib${s}.a")
		endif()
	endforeach()
	set( ${list_name} ${${list_name}_TMP} PARENT_SCOPE)
endfunction()

macro(set_libpath_flag)
	if (WIN64 OR WIN32 OR DURANGO)
		set(LIBPATH_FLAG " /LIBPATH:")
	else()
		set(LIBPATH_FLAG " -L")
	endif()
endmacro()

macro(USE_MSVC_PRECOMPILED_HEADER TargetProject PrecompiledHeader PrecompiledSource)
	if (OPTION_PCH AND MSVC AND NOT OPTION_UNITY_BUILD)
		if (WIN32 OR DURANGO)
			set_target_properties(${TargetProject} PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\"")
			set_source_files_properties(${PrecompiledSource} PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\"")
			# Disable Precompiled Header on all C files
			foreach(C_sourcefile ${SOURCES})
				if (${C_sourcefile} MATCHES ".*\\.\\c$")
					set_property(SOURCE "${C_sourcefile}" APPEND_STRING PROPERTY COMPILE_FLAGS " /Y- ")
				endif ()
			endforeach(C_sourcefile)
			
			# Disable Precompiled headers on QT generated files
			if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${TargetProject}_automoc.cpp")
				set_property( SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${TargetProject}_automoc.cpp" APPEND_STRING PROPERTY COMPILE_FLAGS " /Y- ")
				set_property( SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${TargetProject}_automoc.cpp" PROPERTY "SKIP_AUTOMOC" TRUE)
			endif()
			file(GLOB_RECURSE qrcs ${CMAKE_CURRENT_BINARY_DIR}/qrc_*.cpp)
			foreach(f ${qrcs})
				set_property( SOURCE ${f} APPEND PROPERTY COMPILE_FLAGS "/Y-")
				set_property( SOURCE ${f} PROPERTY "SKIP_AUTOMOC" TRUE)
			endforeach()
		endif()
		
		if (ORBIS)
			#set_target_properties(${TargetProject} PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\"")
			#set_source_files_properties(${PrecompiledSource} PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\"")
		endif()
	endif()
endmacro()

MACRO(EXCLUDE_FILE_FROM_MSVC_PRECOMPILED_HEADER)
	if (MSVC)
		if (WIN32 OR DURANGO)
			set_property( SOURCE ${ARGN} APPEND PROPERTY COMPILE_FLAGS "/Y-")
		endif()
	endif()
ENDMACRO(EXCLUDE_FILE_FROM_MSVC_PRECOMPILED_HEADER)

# Organize projects into solution folders
macro(set_solution_folder folder target)
	if(TARGET ${target})
		set_property(TARGET ${target} PROPERTY FOLDER "${folder}")
	endif()
endmacro()

MACRO(SET_PLATFORM_TARGET_PROPERTIES TargetProject)
	target_compile_definitions( ${THIS_PROJECT} PRIVATE "-DCODE_BASE_FOLDER=\"${CMAKE_SOURCE_DIR}/Code/\"")
	target_link_libraries( ${THIS_PROJECT} PRIVATE ${COMMON_LIBS} )
	IF(DURANGO)
		set_target_properties_for_durango(${TargetProject})
	ENDIF(DURANGO)
	IF(ORBIS)
		set_target_properties_for_orbis(${TargetProject})
	ENDIF(ORBIS)
	
	if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
		get_target_property(archout ${TargetProject} ARCHIVE_OUTPUT_DIRECTORY)
		get_target_property(libout ${TargetProject} LIBRARY_OUTPUT_DIRECTORY)
		get_target_property(runout ${TargetProject} RUNTIME_OUTPUT_DIRECTORY)
		if (NOT archout)
			set(archout ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		endif()
		if (NOT libout)
			set(libout ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		endif()
		if (NOT runout)
			set(runout ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
		endif()

		# Iterate Debug/Release configs and adds _DEBUG or _RELEASE
		foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
			string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
			set_target_properties(${TargetProject} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${archout})
			set_target_properties(${TargetProject} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${libout})
			set_target_properties(${TargetProject} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${runout})
		endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
	endif()
ENDMACRO(SET_PLATFORM_TARGET_PROPERTIES)

# Macro for the Unity Build, creating uber files
function(enable_unity_build UB_FILENAME SOURCE_VARIABLE_NAME)
	if(OPTION_UNITY_BUILD)
		set(files ${${SOURCE_VARIABLE_NAME}})
		
		# Generate a unique filename for the unity build translation unit
		set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/${UB_FILENAME})

		# Add include statement for each translation unit
		foreach(source_file ${files} )
			if (${source_file} MATCHES ".*\\.\\cpp$" OR 
				${source_file} MATCHES ".*\\.\\CPP$" OR
				${source_file} MATCHES ".*\\.\\c$")
				# Exclude from compilation
				set_source_files_properties(${source_file} PROPERTIES HEADER_FILE_ONLY true)
				list(APPEND unit_sources ${source_file})
			endif()
		endforeach(source_file)
		
		# Add target to create uber files
		string(REPLACE ";" "," unit_sources "${unit_sources}")
		add_custom_command(OUTPUT ${unit_build_file}
			           COMMAND ${CMAKE_COMMAND} -DUBER_FILE=${unit_build_file} 
				   			    -DSRC_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
							    -DSRC_FILES="${unit_sources}"
							    -P ${CMAKE_SOURCE_DIR}/Tools/CMake/write_uber_file.cmake)

		# Group Uber files in solution project
		source_group("UBER FILES" FILES ${unit_build_file})
		set_source_files_properties(${unit_build_file} PROPERTIES GENERATED true)

		# Turn off precompiled header
		if (WIN32 OR DURANGO)
			set_source_files_properties(${unit_build_file} PROPERTIES COMPILE_FLAGS "/Y-")
		endif()
	endif()
endfunction(enable_unity_build)

# Process source files
macro(start_sources)
	set(SOURCES)
	set(SOURCE_GROUPS)
	set(UBERFILES)
endmacro()

#Specifies a set of platforms that should build source files provided after this point. Specify ALL to build for all platforms.
#Syntax: sources_platform([OR] X Y Z [AND A B C])
#Subsequent source files will be built if "if(T)" is true for at least one token T in OR and all tokens T in AND.
#Example: sources_platform(WIN32 ANDROID AND HAS_FOO) = build if target is Windows or Android, and HAS_FOO is true.
macro(sources_platform)
	set(PLATFORM_CONDITION)
	set(multiValueArgs OR AND)
	cmake_parse_arguments(COND "" "" "${multiValueArgs}" ${ARGN})
	list(APPEND COND_OR ${COND_UNPARSED_ARGUMENTS})
	set(PLATFORM_CONDITION FALSE)
	foreach(c ${COND_OR})
		if(${c} STREQUAL "ALL")
			set(c TRUE)
		endif()
		if(${c})
			set(PLATFORM_CONDITION TRUE)
		endif()
	endforeach()
	foreach(c ${COND_AND})
		if(${c} STREQUAL "ALL")
			set(c TRUE)
		endif()
		if(NOT ${c})
			set(PLATFORM_CONDITION FALSE)
		endif()
	endforeach()	
endmacro()

macro(add_files)
	foreach(p ${UB_PROJECTS})
		list(APPEND ${p}_SOURCES ${ARGN})
	endforeach()
	list(APPEND SOURCES ${ARGN})
endmacro()

macro(add_to_uberfile uberfile)
	if(OPTION_UNITY_BUILD AND NOT "${uberfile}" STREQUAL "NoUberFile")
		list(APPEND ${uberfile} ${ARGN})
	endif()		
	add_files(${ARGN})
endmacro()

#Usage: add_sources(uberFileName [PROJECTS proj1 proj2 ...] (SOURCE_GROUP "GroupName" file1 file2...)+
#Use "NoUberFile" to signify source files which should not be compiled with others.
#PROJECTS is optional, but should be used when building multiple projects with distinct sets of source files from one directory. If a project is never referenced in PROJECTS, it will use all sources given in this directory.
#Added files will be built if building for a platform matching the latest sources_platform call. If the files should not be built, they are allowed to be missing on the filesystem.
macro(add_sources name)
	set(multiValueArgs PROJECTS SOURCE_GROUP)
	cmake_parse_arguments(UB "" "" "${multiValueArgs}" ${ARGN})

	if(NOT "${name}" STREQUAL "NoUberFile")
		list(APPEND UBERFILES ${name})
		set(${name}_PROJECTS ${UB_PROJECTS})
	endif()

	#Parse source groups manually to avoid conflating multiple groups
	set(CURRENT_SOURCE_GROUP)
	set(EXPECTING_FILE FALSE)
	set(EXPECTING_GROUP_NAME FALSE)
	foreach(ARG ${ARGN})
		if(${ARG} STREQUAL "PROJECTS")
			set(EXPECTING_FILE FALSE)
		elseif(${ARG} STREQUAL "SOURCE_GROUP")
			set(EXPECTING_GROUP_NAME TRUE)
		elseif(EXPECTING_GROUP_NAME)
			set(CURRENT_SOURCE_GROUP ${ARG})
			string(REPLACE " " "_" CURRENT_SOURCE_GROUP_VAR ${CURRENT_SOURCE_GROUP})
			list(FIND SOURCE_GROUPS ${CURRENT_SOURCE_GROUP_VAR} GROUP_INDEX)			
			if(GROUP_INDEX EQUAL -1)
				list(APPEND SOURCE_GROUPS ${CURRENT_SOURCE_GROUP_VAR})
				set(SOURCE_GROUP_${CURRENT_SOURCE_GROUP_VAR})
			endif()
			set(EXPECTING_GROUP_NAME FALSE)
			set(EXPECTING_FILE TRUE)
		elseif(EXPECTING_FILE)
			if(NOT CURRENT_SOURCE_GROUP)
				message(FATAL_ERROR "No source group name defined")
			endif()
			list(APPEND SOURCE_GROUP_${CURRENT_SOURCE_GROUP_VAR} ${ARG})
			if(NOT ${CURRENT_SOURCE_GROUP} STREQUAL "Root")
				source_group(${CURRENT_SOURCE_GROUP} FILES ${ARG})
			endif()
			# .mm files are Objective-C; disable those from build on non-Apple
			if(NOT (${PLATFORM_CONDITION}) OR (NOT APPLE AND ${ARG} MATCHES ".*\\.\\mm$"))
				if (EXISTS ${ARG})
					set_source_files_properties(${ARG} PROPERTIES HEADER_FILE_ONLY TRUE)
					add_files(${ARG})
				endif()
			else()
				add_to_uberfile(${name} ${ARG})
			endif()
		endif()
	endforeach()
endmacro()

macro(get_source_group output group)
	string(REPLACE " " "_" group_var ${group})	
	set(${output} ${SOURCE_GROUP_${group_var}})
endmacro()

macro(end_sources)
	if(OPTION_UNITY_BUILD AND UBERFILES)
		list(REMOVE_DUPLICATES UBERFILES)
		foreach(u ${UBERFILES})
			set(UB_PROJECTS ${${u}_PROJECTS})
			if(UB_PROJECTS)
				list(REMOVE_DUPLICATES UB_PROJECTS)
			endif()
			list(LENGTH UB_PROJECTS projcount)
			if(projcount GREATER 1)
				set(UB_PROJECTS_TEMP ${UB_PROJECTS})
				foreach(UB_PROJECTS ${UB_PROJECTS_TEMP})
					if(${u})
						enable_unity_build(${UB_PROJECTS}_${u} ${u})
						add_files(${UB_PROJECTS}_${u})
					endif()
				endforeach()
			else()
				if(${u})
					enable_unity_build(${u} ${u})
					add_files(${u})
				endif()
			endif()
		endforeach()
	endif()
endmacro()

# Shared settings for specific module types
macro(force_static_crt)
	target_compile_definitions(${THIS_PROJECT} PUBLIC -D_MT)
	if(MSVC)
		target_compile_options(${THIS_PROJECT} PUBLIC /MT$<$<CONFIG:Debug>:d>)
	endif()
endmacro()

macro(read_settings)
	set(options DISABLE_MFC)
	set(oneValueArgs SOLUTION_FOLDER PCH)
	set(multiValueArgs FILE_LIST INCLUDES LIBS DEFINES)
	cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
endmacro()

macro(prepare_project)
	set(THIS_PROJECT ${target} PARENT_SCOPE)
	set(THIS_PROJECT ${target})
	include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
	project(${target})
	read_settings(${ARGN})
	if(NOT ${THIS_PROJECT}_SOURCES)
		set(${THIS_PROJECT}_SOURCES ${SOURCES})
	endif()	
endmacro()

macro(apply_compile_settings)
	if (MODULE_PCH)
		string(REPLACE ".cpp" ".h" PCH_H ${MODULE_PCH})
		get_filename_component(PCH_H ${PCH_H} NAME)
		USE_MSVC_PRECOMPILED_HEADER( ${THIS_PROJECT} ${PCH_H} ${MODULE_PCH} )
		set_property(TARGET ${THIS_PROJECT} APPEND PROPERTY AUTOMOC_MOC_OPTIONS -b ${PCH_H})
	endif()
	
	SET_PLATFORM_TARGET_PROPERTIES( ${THIS_PROJECT} )	
	if(MODULE_SOLUTION_FOLDER)
		set_solution_folder("${MODULE_SOLUTION_FOLDER}" ${THIS_PROJECT})
	endif()	
endmacro()

function(CryEngineModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	use_scaleform()
	apply_compile_settings()
	if (OPTION_STATIC_LINKING)
		target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB -DCRY_IS_MONOLITHIC_BUILD)
		set(MODULES ${MODULES} ${THIS_PROJECT} CACHE INTERNAL "Modules for monolithic builds" FORCE)
	endif()

	install(TARGETS ${target} LIBRARY DESTINATION bin RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
endfunction()

function(CryGameModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	use_scaleform()
	apply_compile_settings()
	if (OPTION_STATIC_LINKING)
		target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB -DCRY_IS_MONOLITHIC_BUILD)
		set(GAME_MODULES ${GAME_MODULES} ${THIS_PROJECT} CACHE INTERNAL "Game Modules for monolithic builds" FORCE)
	endif()

	install(TARGETS ${target} LIBRARY DESTINATION bin RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
endfunction()

function(CreateDynamicModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} SHARED ${${THIS_PROJECT}_SOURCES})
	apply_compile_settings()
endfunction()

function(CryEngineStaticModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} STATIC ${${THIS_PROJECT}_SOURCES})
	target_compile_definitions(${THIS_PROJECT} PRIVATE -D_LIB)
	apply_compile_settings()
endfunction()

function(CryLauncher target)
	prepare_project(${ARGN})
	if(ANDROID)
		add_library(${target} SHARED ${${THIS_PROJECT}_SOURCES})
		configure_android_launcher(${target})
		file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/${target}.vcxproj.user" 
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>
    <Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
		<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Tegra-Android'\">
			<OverrideAPKPath>$(OutDir)/AndroidLauncher.apk</OverrideAPKPath>		
			<AdditionalLibraryDirectories>$(OutDir)/lib_debug/armeabi-v7a</AdditionalLibraryDirectories>
			<BuildXmlPath>$(OutDir)</BuildXmlPath>
			<GdbSetupPath>$(OutDir)</GdbSetupPath>
			<DebuggerFlavor>AndroidDebugger</DebuggerFlavor>
			<AndroidDebugMode>all</AndroidDebugMode>
    </PropertyGroup>
    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Profile|Tegra-Android'\">
			<OverrideAPKPath>$(OutDir)/AndroidLauncher.apk</OverrideAPKPath>
			<AdditionalLibraryDirectories>$(OutDir)/lib_debug/armeabi-v7a</AdditionalLibraryDirectories>
			<BuildXmlPath>$(OutDir)</BuildXmlPath>
			<GdbSetupPath>$(OutDir)</GdbSetupPath>
			<DebuggerFlavor>AndroidDebugger</DebuggerFlavor>
			<AndroidDebugMode>all</AndroidDebugMode>
    </PropertyGroup>		
    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Tegra-Android'\">
			<OverrideAPKPath>$(OutDir)/AndroidLauncher.apk</OverrideAPKPath>
			<AdditionalLibraryDirectories>$(OutDir)/lib_debug/armeabi-v7a</AdditionalLibraryDirectories>
			<BuildXmlPath>$(OutDir)</BuildXmlPath>
			<GdbSetupPath>$(OutDir)</GdbSetupPath>
			<DebuggerFlavor>AndroidDebugger</DebuggerFlavor>
			<AndroidDebugMode>all</AndroidDebugMode>
    </PropertyGroup>
    </Project>")		
	elseif(WIN32)
		add_executable(${THIS_PROJECT} WIN32 ${${THIS_PROJECT}_SOURCES})
	else()
		add_executable(${target} ${${THIS_PROJECT}_SOURCES})
	endif()
	if(ORBIS)
		set_property(TARGET ${target} PROPERTY OUTPUT_NAME "${target}.elf")	
	elseif(NOT ANDROID)
		set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "Game")	
	endif()
	if(OPTION_STATIC_LINKING)
		use_scaleform()
		target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB -DCRY_IS_MONOLITHIC_BUILD)
		target_link_libraries(${THIS_PROJECT} PRIVATE ${MODULES})
	endif()
	apply_compile_settings()	

	if(NOT ANDROID)
		install(TARGETS ${target} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
	endif()
endfunction()

function(CryDedicatedServer target)
	prepare_project(${ARGN})
	if(WIN32)
		add_executable(${THIS_PROJECT} WIN32 ${${THIS_PROJECT}_SOURCES})
	else()
		add_executable(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	endif()
	set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "Game_Server")
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:WINDOWS")	
	apply_compile_settings()	
endfunction()

function(CryConsoleApplication target)
	prepare_project(${ARGN})
	add_executable(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:CONSOLE")
	apply_compile_settings()	
endfunction()

function(CryFileContainer target)
	prepare_project(${ARGN})
	add_custom_target( ${THIS_PROJECT} SOURCES ${${THIS_PROJECT}_SOURCES})
	if(MODULE_SOLUTION_FOLDER)
		set_solution_folder("${MODULE_SOLUTION_FOLDER}" ${THIS_PROJECT})
	endif()
endfunction()

macro(set_editor_flags)
	target_include_directories( ${THIS_PROJECT} PRIVATE
		${EDITOR_DIR}
		${EDITOR_DIR}/Include	
		${CMAKE_SOURCE_DIR}/Code/Sandbox/Plugins/EditorCommon 		
		${CMAKE_SOURCE_DIR}/Code/CryEngine/CryCommon 
		${SDK_DIR}/boost
		${SDK_DIR}/yasli
		${CRY_LIBS_DIR}/yasli
	)
	if(OPTION_SANDBOX STREQUAL "Qt")
		target_compile_definitions( ${THIS_PROJECT} PRIVATE -DSANDBOX_QT )
	endif()
	target_compile_definitions( ${THIS_PROJECT} PRIVATE
		-DWIN32
		-DCRY_ENABLE_RC_HELPER
		-DIS_EDITOR_BUILD
		-DQT_FORCE_ASSERT
		-DUSE_PYTHON_SCRIPTING 
	)
	if(NOT MODULE_DISABLE_MFC)
		target_compile_definitions( ${THIS_PROJECT} PRIVATE -D_AFXDLL)
	endif()
	target_link_libraries( ${THIS_PROJECT} PRIVATE yasli BoostPython python27)
	use_qt()
endmacro()

function(CryEditor target)
	prepare_project(${ARGN})
	if(WIN32)
		add_executable(${THIS_PROJECT} WIN32 ${${THIS_PROJECT}_SOURCES})
	else()
		add_executable(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	endif()
	set_editor_flags()
	target_compile_options(${THIS_PROJECT} PRIVATE /EHsc /GR /bigobj /Zm200 /wd4251 /wd4275)
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DSANDBOX_EXPORTS -DPLUGIN_IMPORTS -DEDITOR_COMMON_IMPORTS)
	target_link_libraries(${THIS_PROJECT} PRIVATE EditorCommon)
	set_property(TARGET ${THIS_PROJECT} PROPERTY ENABLE_EXPORTS TRUE)
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:WINDOWS")
	apply_compile_settings()
endfunction()

function(CryPlugin target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	set_editor_flags()
	target_compile_options(${THIS_PROJECT} PRIVATE /EHsc /GR /wd4251 /wd4275)
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DSANDBOX_IMPORTS -DPLUGIN_EXPORTS -DEDITOR_COMMON_IMPORTS -DNOT_USE_CRY_MEMORY_MANAGER)
	if (OPTION_SANDBOX STREQUAL "Qt")
		set_property(TARGET ${THIS_PROJECT} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPlugins)
		set_property(TARGET ${THIS_PROJECT} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPlugins)
		set_property(TARGET ${THIS_PROJECT} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPlugins)
		target_link_libraries(${THIS_PROJECT} PRIVATE Sandbox EditorCommon)
	else()
		set_property(TARGET ${THIS_PROJECT} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPluginsLegacy)
		set_property(TARGET ${THIS_PROJECT} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPluginsLegacy)
		set_property(TARGET ${THIS_PROJECT} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EditorPluginsLegacy)		
		target_link_libraries(${THIS_PROJECT} PRIVATE SandboxLegacy EditorCommon)
	endif()
	apply_compile_settings()	
endfunction()

function(CryPluginModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	set_editor_flags()
	target_compile_options(${THIS_PROJECT} PRIVATE /EHsc /GR /wd4251 /wd4275)
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DPLUGIN_EXPORTS -DEDITOR_COMMON_EXPORTS -DNOT_USE_CRY_MEMORY_MANAGER)
	if ("${target}" STREQUAL "EditorCommon" AND NOT OPTION_SANDBOX STREQUAL "Qt")
		set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "EditorCommonLegacy")
	endif()	
	apply_compile_settings()
endfunction()

macro(set_rc_flags)
	# Forcing static crt gets Linker Tools Error LNK2038. 
	# https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx All modules passed to a given invocation of the linker must have been compiled with the same run-time library compiler option.
	#
	# force_static_crt()
	
	target_compile_definitions( ${THIS_PROJECT} PRIVATE
		-DWIN32
		-DRESOURCE_COMPILER
		-DFORCE_STANDARD_ASSERT
		-DNOT_USE_CRY_MEMORY_MANAGER
	)
	target_include_directories( ${THIS_PROJECT} PRIVATE 
		${CMAKE_SOURCE_DIR}/Code/CryEngine/CryCommon 
		${SDK_DIR}/boost
		${SDK_DIR}/yasli
		${CRY_LIBS_DIR}/yasli
		${CMAKE_SOURCE_DIR}/Code/Sandbox/Plugins/EditorCommon 
	)
	target_link_libraries( ${THIS_PROJECT} PRIVATE yasli )
endmacro()

function(CryResourceCompiler target)
	prepare_project(${ARGN})
	if(WIN32)
		add_executable(${THIS_PROJECT} WIN32 ${${THIS_PROJECT}_SOURCES})
	else()
		add_executable(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	endif()
	set_rc_flags()
	if(WIN32)
		set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:CONSOLE")
	endif()
	set_property(TARGET ${THIS_PROJECT} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	set_property(TARGET ${THIS_PROJECT} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)	
	set_property(TARGET ${THIS_PROJECT} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)	
	set_property(TARGET ${THIS_PROJECT} PROPERTY OUTPUT_NAME "rc")
	apply_compile_settings()	
endfunction()

function(CryResourceCompilerModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	set_rc_flags()
	set_property(TARGET ${THIS_PROJECT} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	set_property(TARGET ${THIS_PROJECT} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	set_property(TARGET ${THIS_PROJECT} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	apply_compile_settings()
endfunction()

macro(set_pipeline_flags)
	force_static_crt()
	target_compile_definitions( ${THIS_PROJECT} PRIVATE
		-DWIN32
		-DRESOURCE_COMPILER
		-DFORCE_STANDARD_ASSERT
		-DNOT_USE_CRY_MEMORY_MANAGER
	)
	target_include_directories( ${THIS_PROJECT} PRIVATE 
		${CMAKE_SOURCE_DIR}/Code/CryEngine/CryCommon 
	)
endmacro()

function(CryPipelineModule target)
	prepare_project(${ARGN})
	add_library(${THIS_PROJECT} ${${THIS_PROJECT}_SOURCES})
	set_rc_flags()
	set_property(TARGET ${THIS_PROJECT} PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	set_property(TARGET ${THIS_PROJECT} PROPERTY LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	set_property(TARGET ${THIS_PROJECT} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tools/rc)
	if(WIN32)
		set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:CONSOLE")
	endif()
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:msvcprtd.lib)
	apply_compile_settings()	
endfunction()

# WAF features
macro(use_qt)
	#custom_enable_QT5()
	set(CMAKE_PREFIX_PATH "${QT_DIR}")
	set_property(TARGET ${THIS_PROJECT} PROPERTY AUTOMOC TRUE)
	set_property(TARGET ${THIS_PROJECT} PROPERTY AUTORCC TRUE)

	target_compile_definitions(${THIS_PROJECT} PRIVATE -DQT_GUI_LIB -DQT_NO_EMIT -DQT_WIDGETS_LIB)
	target_include_directories(${THIS_PROJECT} PRIVATE ${QT_DIR}/include)
	set_libpath_flag()
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " ${LIBPATH_FLAG}${QT_DIR}/lib")

	set(QT_MODULES Core Gui OpenGL Widgets)
	foreach(MODULE ${QT_MODULES})
		#find_package(Qt5${MODULE} REQUIRED)
		target_include_directories(${THIS_PROJECT} PRIVATE ${QT_DIR}/include/Qt${MODULE})
		target_link_libraries(${THIS_PROJECT} PRIVATE Qt5${MODULE}$<$<CONFIG:Debug>:d>)
	endforeach()
endmacro()

macro(process_csharp output_module)
	set(CMAKE_MODULE_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_PROFILE})
	set(swig_inputs)
	set(swig_globals)
	foreach(sourcefile ${SOURCES})
		if (${sourcefile} MATCHES ".*\\.\\i$")
			set(swig_inputs ${swig_inputs} ${sourcefile})
		endif()
		if (${sourcefile} MATCHES ".*\\.\\swig$")
			set(swig_globals ${swig_globals} ${sourcefile})
		endif()
	endforeach()

	set(SWIG_EXECUTABLE ${SDK_DIR}/swig/swig)
	set(mono_inputs)

	foreach(f ${swig_inputs})
		string(LENGTH ${f} flen)
		math(EXPR flen ${flen}-2)
		string(SUBSTRING ${f} 0 ${flen} basename)
		set(basename ${CMAKE_CURRENT_BINARY_DIR}/${basename})
		set(f_cpp ${basename}.cpp)
		set(f_cs ${basename}.cs)
		set(f_h ${basename}.h)
		if(WIN64)
			set(defs -D_WIN32 -D_WIN64)
		elseif(WIN32)
			set(defs -D_WIN32)
		else()
			message(ERROR "Mono not supported on this platform")
		endif()

		get_target_property(target_defs ${THIS_PROJECT} COMPILE_DEFINITIONS)
		foreach(d ${target_defs})
			set(defs ${defs} -D${d})
		endforeach()
		set(defs ${defs} -D_MT -D_DLL -D_USRDLL)

		get_filename_component(f_cs_dir ${f_cs} DIRECTORY)
		get_filename_component(f_cs_name ${f_cs} NAME)

		# Detect dependencies
		execute_process(
			COMMAND ${SWIG_EXECUTABLE} -MM ${defs} -csharp ${CMAKE_CURRENT_SOURCE_DIR}/${f}
			OUTPUT_VARIABLE swig_deps
		)
		string(REGEX MATCHALL "\n  [^ ][^ ][^ :]+" temp ${swig_deps})
		set(swig_deps)
		foreach(t ${temp})
			string(STRIP "${t}" t)
			set(swig_deps ${swig_deps} "${t}")
		endforeach()

		set(defs ${defs} -D_$<UPPER_CASE:$<CONFIG>> -D$<UPPER_CASE:$<CONFIG>>)

		set(mono_inputs ${mono_inputs} ${f_cs})
		add_custom_command(
			OUTPUT "${f_cpp}" "${f_h}" "${f_cs}"
			COMMAND ${SWIG_EXECUTABLE} -c++ ${defs} -DSWIG_CSHARP_NO_IMCLASS_STATIC_CONSTRUCTOR ${secondary_defs} -csharp -o ${f_cpp} -outdir ${f_cs_dir} -outfile ${f_cs_name} -namespace ${output_module} -pch-file "\\\"StdAfx.h\\\"" -fno-include-guards -dllimport ${THIS_PROJECT} ${CMAKE_CURRENT_SOURCE_DIR}/${f}
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${f}
			DEPENDS ${swig_deps}
		)
	set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${swig_deps} ${CMAKE_CURRENT_SOURCE_DIR}/${f})
		set(secondary_defs -DSWIG_CXX_EXCLUDE_SWIG_INTERFACE_FUNCTIONS -DSWIG_CSHARP_EXCLUDE_STRING_HELPER -DSWIG_CSHARP_EXCLUDE_EXCEPTION_HELPER)
		target_sources(${THIS_PROJECT} PRIVATE ${f_cpp} ${f_h})
		source_group("Generated" FILES ${f_cpp} ${f_h})
		set_source_files_properties(${f_h} PROPERTIES HEADER_FILE_ONLY true GENERATED true)
	endforeach()

	set(mono_path ${SDK_DIR}/Mono/bin/mcs)
	set(mono_lib_path ${SDK_DIR}/Mono/lib/mono)

	#TODO: Metadata
	add_custom_command(
		TARGET ${THIS_PROJECT} PRE_LINK
		COMMAND ${mono_path} -target:library -langversion:4 -platform:anycpu -optimize -g -L ${mono_lib_path} ${mono_inputs} -out:${OUTPUT_DIRECTORY}/${output_module}.dll
		DEPENDS ${mono_inputs}
	)

endmacro()


# Module extensions from WAF
macro(use_mono)
	if(WIN32)
		target_compile_definitions(${THIS_PROJECT} PRIVATE -DUSE_MONO_BRIDGE)
		target_include_directories(${THIS_PROJECT} PRIVATE ${SDK_DIR}/Mono/include/mono-2.0)
		set_libpath_flag()
		if (WIN64)
			set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " ${LIBPATH_FLAG}${SDK_DIR}/Mono/lib/x64")
		else()
			set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " ${LIBPATH_FLAG}${SDK_DIR}/Mono/lib/x86")
		endif()		
	endif()
endmacro()


macro(use_scaleform)
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DCRY_FEATURE_SCALEFORM_HELPER)
	if (EXISTS ${SDK_DIR}/Scaleform)
		target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}/Scaleform/Include" )
		target_compile_definitions(${THIS_PROJECT} PRIVATE -DINCLUDE_SCALEFORM_SDK)
		set(SCALEFORM_LIB_FOLDER "${SDK_DIR}/Scaleform/Lib")
		set(SCALEFORM_HAS_SHIPPING_LIB TRUE)
		if (WIN64)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/${MSVC_LIB_PREFIX}/Win64 )
			set(SCALEFORM_LIBS libgfx)
		elseif(WIN32)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/${MSVC_LIB_PREFIX}/Win32 )
			set(SCALEFORM_LIBS libgfx)
		elseif(DURANGO)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/${MSVC_LIB_PREFIX}/Durango )
			set(SCALEFORM_LIBS libgfx)
		elseif(LINUX)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/linux)
			set(SCALEFORM_HAS_SHIPPING_LIB FALSE)
			set(SCALEFORM_LIBS gfx jpeg png16)
		elseif(ORBIS)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/ORBIS)
			set(SCALEFORM_LIBS gfx gfx_video)
		elseif(ANDROID)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/android-armeabi-v7a)
			set(SCALEFORM_HAS_SHIPPING_LIB FALSE)
			set(SCALEFORM_LIBS gfx)
		elseif(APPLE)
			set(SCALEFORM_LIB_FOLDER ${SCALEFORM_LIB_FOLDER}/mac)
			set(SCALEFORM_HAS_SHIPPING_LIB FALSE)
			set(SCALEFORM_LIBS gfx jpeg png16)
		endif()

		if(SCALEFORM_HAS_SHIPPING_LIB)
			set(SCALEFORM_RELEASE_CONFIG Shipping)
			set_property(TARGET ${THIS_PROJECT} APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Release>:-DGFC_BUILD_SHIPPING>)
		else()
			set(SCALEFORM_RELEASE_CONFIG Release)
		endif()

		# Set linker search path
		set_libpath_flag()
		if(APPLE OR LINUX)
			set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS "${LIBPATH_FLAG}${SCALEFORM_LIB_FOLDER}")
		endif()
		set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "${LIBPATH_FLAG}${SCALEFORM_LIB_FOLDER}/Debug/")
		set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_PROFILE "${LIBPATH_FLAG}${SCALEFORM_LIB_FOLDER}/Release/")
		set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE "${LIBPATH_FLAG}${SCALEFORM_LIB_FOLDER}/${SCALEFORM_RELEASE_CONFIG}/")

		target_link_libraries(${THIS_PROJECT} PRIVATE ${SCALEFORM_LIBS})
	endif()
endmacro()

macro(use_xt)
	if (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
		set(XT_VERSION vc14)	
	elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
		set(XT_VERSION vc12)
	elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
		set(XT_VERSION vc11)
	endif()
	target_include_directories( ${THIS_PROJECT} PRIVATE ${SDK_DIR}/XT_13_4/Include )
	set_libpath_flag()
	set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " ${LIBPATH_FLAG}${SDK_DIR}/XT_13_4/lib_${XT_VERSION}")
endmacro()

