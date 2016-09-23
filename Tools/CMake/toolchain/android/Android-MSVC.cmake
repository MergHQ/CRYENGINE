set(CMAKE_CROSSCOMPILING "TRUE")

# https://blogs.msdn.microsoft.com/vcblog/2015/12/15/support-for-android-cmake-projects-in-visual-studio/
set(ANDROID TRUE)
set(CMAKE_SYSTEM_NAME "VCMDDAndroid")
set(CMAKE_SYSTEM_VERSION 2.0)
set(CMAKE_ANDROID_API 21)
set(CMAKE_ANDROID_API_MIN 17)

set(VC_MDD_ANDROID_API_LEVEL "android-${CMAKE_ANDROID_API}")
set(VC_MDD_ANDROID_PLATFORM_TOOLSET "Clang_3_8") # "Clang_3_6","Clang_3_8","Gcc_4_9"
#set(VC_MDD_ANDROID_USE_OF_STL "c++_static")

set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/android")

set(CMAKE_CONFIGURATION_TYPES Debug Profile Release)
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)

# Set Visual Studio NDK Root
GET_FILENAME_COMPONENT(NDKROOT "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\VisualStudio\\14.0\\Setup\\vs\\SecondaryInstaller\\AndroidNDK64\\2.0;NDK_HOME]" ABSOLUTE CACHE)

# Add additional include directories
include_directories( ${NDKROOT}/sources/android/support/include )
include_directories( ${NDKROOT}/sources/android/native_app_glue )
include_directories( ${CMAKE_SOURCE_DIR}/Code/Tools/SDLExtension/src/include )

set(OPTION_STATIC_LINKING TRUE)

if("${VC_MDD_ANDROID_PLATFORM_TOOLSET}" MATCHES "Clang" OR ${VC_MDD_ANDROID_PLATFORM_TOOLSET} EQUAL 0)
	MESSAGE(STATUS "COMPILER = CLANG" )
	include (${CMAKE_MODULE_PATH}/../CRYENGINE-CLANG.cmake)	
elseif("${VC_MDD_ANDROID_PLATFORM_TOOLSET}" MATCHES "Gcc")
	MESSAGE(STATUS "COMPILER = GCC" )
	include (${CMAKE_MODULE_PATH}/../CRYENGINE-GCC.cmake)
else()
	MESSAGE(FATAL_ERROR "COMPILER: Unsupported Compiler detected" )
endif()

set (ANDROID_COMMON_FLAGS 
	-mfpu=neon 
	-mfloat-abi=softfp 
	-march=armv7-a
	-marm	
	-ffunction-sections 
	-fdata-sections
	
	-DANDROID_NDK
	-DANDROID 
	-DDISABLE_IMPORTGL 
	-D__ANDROID__  
	-D__ARM_ARCH_7A__ 
	-D_LINUX 
	-DLINUX 
	-DLINUX32 
	-DCRY_MOBILE
	-D_MT 
	-D_DLL 
	-D_PROFILE 
	-DPROFILE
	)
	string(REPLACE ";" " " ANDROID_COMMON_FLAGS "${ANDROID_COMMON_FLAGS}")
	
if(CMAKE_ANDROID_API GREATER 20)
	SET(ANDROID_COMMON_FLAGS "${ANDROID_COMMON_FLAGS} -DHAS_STPCPY=1")
endif()
	
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ANDROID_COMMON_FLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ANDROID_COMMON_FLAGS}")

MESSAGE(STATUS "BUILD_CPU_ARCHITECTURE = ${BUILD_CPU_ARCHITECTURE}" )
MESSAGE(STATUS "BUILD_PLATFORM = ${BUILD_CPU_ARCHITECTURE}" )

# Build whole archives only on android
set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--whole-archive -Wl,-Bdynamic -Wl,--allow-multiple-definition" )
set( CMAKE_STATIC_LINKER_FLAGS "-Wl,--whole-archive -Wl,-Bstatic -Wl,--allow-multiple-definition" )
set( CMAKE_EXE_LINKER_FLAGS " -Wl,--allow-multiple-definition" )
	
if (CMAKE_CL_64 OR ${CMAKE_GENERATOR} MATCHES "ARM64")
	message ("ARM64 not supported.")
	# TO DO:
	#set(BUILD_CPU_ARCHITECTURE "ARM")
	#set(BUILD_PLATFORM "armeabi-v7a")
	#set (ANDROID_COMMON_FLAGS "${ANDROID_COMMON_FLAGS} -DLINUX64"
elseif(CMAKE_CL_32 OR ${CMAKE_GENERATOR} MATCHES "ARM")
	set(BUILD_CPU_ARCHITECTURE "ARM")
	set(BUILD_PLATFORM "armeabi-v7a") #https://developer.android.com/ndk/guides/abis.html
endif()

# Profile flags
set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_CXX_FLAGS} CACHE STRING "C++ Flags for Profile" FORCE)
set(CMAKE_C_FLAGS_PROFILE ${CMAKE_C_FLAGS} CACHE STRING "C Flags for Profile" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "" CACHE STRING "Linker Flags for Profile" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "/debug /INCREMENTAL" CACHE STRING "C++ Flags" FORCE)

# Release flags
#set(CMAKE_CXX_FLAGS_RELEASE "/MD /Ox /Oi /Ot /Ob2 /Oy- /GS- /D NDEBUG /D_RELEASE /D PURE_CLIENT" CACHE STRING "C++ Flags" FORCE)
#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "C++ Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "C++ Flags" FORCE)
# -------------------------------------------------------------------



macro(configure_android_build)
	set(options DEBUGGABLE)
	set(oneValueArgs PACKAGE APP_NAME VERSION_CODE VERSION_NAME)
	set(multiValueArgs PERMISSIONS)
	cmake_parse_arguments(ANDROID "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	string(TOLOWER "${ANDROID_DEBUGGABLE}" ANDROID_DEBUGGABLE) 
	
	set(apk_folder "${CMAKE_BINARY_DIR}/apk_data")
	file(REMOVE_RECURSE ${apk_folder})
	file(MAKE_DIRECTORY ${apk_folder}/src)
	file(MAKE_DIRECTORY ${apk_folder}/lib/armeabi-v7a)
	file(MAKE_DIRECTORY ${apk_folder}/gen)
	file(MAKE_DIRECTORY ${apk_folder}/bin)
	file(MAKE_DIRECTORY ${apk_folder}/build)

	# TODO: implement process_android_java_files from waf compile rules

	#Generate APK manifest
	file(WRITE ${apk_folder}/AndroidManifest.xml 
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"	<!-- BEGIN_INCLUDE(manifest) -->\n"
		"	<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n"
		"		package=\"${ANDROID_PACKAGE}\"\n"
		"		android:versionCode=\"${ANDROID_VERSION_CODE}\"\n"
		"		android:debuggable=\"${ANDROID_DEBUGGABLE}\"\n"
		"		android:versionName=\"${ANDROID_VERSION_CODE}\" >\n"
		"\n"
		"		<!-- This is the platform API where NativeActivity was introduced. -->\n"
		"		<uses-sdk android:minSdkVersion=\"${CMAKE_ANDROID_API_MIN}\" />\n"
		"\n"
		"		<!-- OpenGL ES 3.0 -->\n"
		"		<uses-feature android:glEsVersion=\"0x00030000\" android:required=\"true\" /> \n"
		"\n"
		"		<!-- Required texture compression support -->\n"
		"		<supports-gl-texture android:name=\"GL_IMG_texture_compression_pvrtc\" />\n"
		"		<supports-gl-texture android:name=\"GL_EXT_texture_compression_s3tc\" />\n"
		"\n"
		"		<!-- Required permissions -->\n")
	foreach(permission ${ANDROID_PERMISSIONS})
		file(APPEND ${apk_folder}/AndroidManifest.xml 
			"		<uses-permission android:name=\"android.permission.${permission}\" /> \n"
		)
	endforeach()
	file(APPEND ${apk_folder}/AndroidManifest.xml 
		"\n"
		"		<application android:label=\"@string/app_name\">\n"
		"			<!-- Our activity loads the generated bootstrapping class in order\n"
		"				to prelaod any third party shared libraries -->\n"
		"			<activity android:name=\"CryEngineActivity\"\n"
		"				android:label=\"@string/app_name\"\n"
		"				android:theme=\"@android:style/Theme.NoTitleBar.Fullscreen\"\n"
		"				android:screenOrientation=\"landscape\"\n"
		"				android:configChanges=\"orientation|keyboardHidden\">\n"
		"\n"
		"				<!-- Tell NativeActivity the name of our .so -->\n"
		"				<meta-data android:name=\"android.app.lib_name\" android:value=\"${THIS_PROJECT}Launcher\" />\n"
		"				<intent-filter>\n"
		"					<action android:name=\"android.intent.action.MAIN\" />\n"
		"					<category android:name=\"android.intent.category.LAUNCHER\" />\n"
		"				</intent-filter>\n"
		"			</activity>\n"
		"		</application>\n"
		"\n"
		"	</manifest> \n"
		"	<!-- END_INCLUDE(manifest) -->\n"
	)

	#Generate resources
	file(MAKE_DIRECTORY ${apk_folder}/res/values)
	file(WRITE ${apk_folder}/res/values/string.xml
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<resources>\n"
		"	<string name=\"app_name\">${ANDROID_APP_NAME}</string>\n"
		"</resources>\n"
	)

	#Generate gdb.setup
	file(WRITE ${OUTPUT_DIRECTORY}/gdb.setup "set solib-search-path ${OUTPUT_DIRECTORY}/lib_debug/armeabi-v7a")
	
	#Generate ANT files
	file(WRITE ${apk_folder}/build.xml
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"	<project name=\"${THIS_PROJECT}Launcher\" default=\"help\">\n"
		"\n"
		"		<property file=\"build.properties\" />\n"
		"\n"
		"		<!-- quick check on sdk.dir -->\n"
		"		<fail	message=\"sdk.dir is missing. Make sure to generate local.properties using 'android update project' or to inject it through the ANDROID_HOME environment variable.\"\n"
		"				unless=\"sdk.dir\" />\n"
		"\n"
		"		<!-- Import the actual build file.\n"
		"\n"
		"			To customize existing targets, there are two options:\n"
		"			1) Customize only one target:\n"
		"				- copy/paste the target into this file, *before* the <import> task.\n"
		"				#NAME?\n"
		"			2) Customize the whole content of build.xml\n"
		"				- copy/paste the content of the rules files (minus the top node) into this file, replacing the <import> task.\n"
		"				#NAME?\n"
		"\n"
		"			***********************\n"
		"			****** IMPORTANT ******\n"
		"			***********************\n"
		"			In all cases you must update the value of version-tag below to read 'custom' instead of an integer,\n"
		"			in order to avoid having your file be overridden by tools such as \"android update project\"\n"
		"		-->\n"
		"\n"
		"		<!-- version-tag: 1 -->\n"
		"		<import file=\"\\\${sdk.dir}/tools/ant/build.xml\" />\n"
		"	</project>\n"		
	)

	file(TO_CMAKE_PATH "$ENV{ANDROID_HOME}" ANDROID_SDK_DIR)

	file(WRITE ${apk_folder}/build.properties 
		"# This file is automatically generated by Android Tools.\n"
		"# Do not modify this file -- YOUR CHANGES WILL BE ERASED!\n"
		"\n"
		"target=android-${CMAKE_ANDROID_API}\n"
		"\n"
		"gen.absolute.dir=./gen\n"
		"out.dir=./bin\n"
		"jar.libs.dir=./libs\n"
		"native.libs.absolute.dir=./lib\n"
		"sdk.dir=${ANDROID_SDK_DIR}\n"
	)

endmacro()

macro(configure_android_launcher)
	set(apk_folder "${CMAKE_BINARY_DIR}/apk_data")

	#Copy sources
	foreach(source_file ${SOURCES})
		if (${source_file} MATCHES ".*\\.\\java$")
			file(COPY ${source_file} DESTINATION ${apk_folder}/src)
		endif()
	endforeach()

	#Copy external libs
	file(COPY ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/libgnustl_shared.so DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${NDKROOT}/prebuilt/android-arm/gdbserver/gdbserver DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${CMAKE_SOURCE_DIR}/Code/Tools/SDLExtension/lib/android-armeabi-v7a/libSDL2Ext.so DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${SDK_DIR}/SDL2/lib/android-armeabi-v7a/libSDL2.so DESTINATION ${apk_folder}/lib/armeabi-v7a)

	#Make ANT run
	file(TO_NATIVE_PATH "${OUTPUT_DIRECTORY}" NATIVE_OUTDIR)
	file(TO_NATIVE_PATH "${apk_folder}" apk_folder_native)

	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<Project DefaultTargets=\"Build\" ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
		"<ItemGroup Label=\"ProjectConfigurations\">\n"
	)
		
	foreach(config ${CMAKE_CONFIGURATION_TYPES})
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj	
				"<ProjectConfiguration Include=\"${config}|ARM\">\n"
					"<Configuration>${config}</Configuration>\n"
					"<Platform>ARM</Platform>\n"
				"</ProjectConfiguration>"			
		)
	endforeach()
		
	set(UUID_NAMESPACE E07D42D6-FAC9-420E-B260-25549254EDCA)
	string(UUID UUID_PACKAGING_PROJECT NAMESPACE ${UUID_NAMESPACE} NAME ${THIS_PROJECT} TYPE MD5)
	file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj	 
		"</ItemGroup>\n"
		"<PropertyGroup Label=\"Globals\">\n"
			"<MinimumVisualStudioVersion>14.0</MinimumVisualStudioVersion>\n"
			"<ProjectVersion>1.0</ProjectVersion>\n"
			"<ProjectGuid>{${UUID_PACKAGING_PROJECT}}</ProjectGuid>\n"
		"</PropertyGroup>\n"
		
		"<Import Project=\"$(AndroidTargetsPath)\\Android.Default.props\" />\n"
		
	)
	foreach(config ${CMAKE_CONFIGURATION_TYPES})
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj	
			"<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='${config}|ARM'\" Label=\"Configuration\">\n"
				"<UseDebugLibraries>true</UseDebugLibraries>\n"
				"<ConfigurationType>Application</ConfigurationType>\n"
				"<OutDir></OutDir>\n"
			"</PropertyGroup>\n"
		)	
	endforeach()
		
	file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj
		"<Import Project=\"$(AndroidTargetsPath)\\Android.props\" />\n"
		"<ImportGroup Label=\"ExtensionSettings\" />\n"
		"<ImportGroup Label=\"Shared\" />\n"
		"<PropertyGroup Label=\"UserMacros\" />\n"
	  
		"<Import Project=\"$(AndroidTargetsPath)\\Android.targets\" />\n"
		"<ImportGroup Label=\"ExtensionTargets\" />\n"
		
		"<!-- Override MSBUILD ANT build command with nothing -->"
		"<Target Name = \"AntPackage\">\n"
		"</Target>\n"
		
		"<Target Name=\"AntPackageClean\">\n"
		"</Target>\n"
	
		"</Project>\n"
	)

	set(WIN32 TRUE) # include_external_msproject only works if this is set, so do so temporarily
	include_external_msproject(${THIS_PROJECT}.Packaging ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj
                           TYPE 39E2626F-3545-4960-A6E8-258AD8476CE5
                           GUID ${UUID_PACKAGING_PROJECT}
                           PLATFORM "ARM" )
	unset(WIN32)
	set_solution_folder("Launcher" ${THIS_PROJECT}.Packaging)
							   						   
	#SET(${PROJECT_NAME}_GUID_CMAKE "<GUID>" CACHE INTERNAL "Project GUID")
	
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj.user
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<Project ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
	)
	
	foreach(config ${CMAKE_CONFIGURATION_TYPES})
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj.user
		"<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='${config}|ARM'\">\n"
		"<PackagePath>${OUTPUT_DIRECTORY}/${THIS_PROJECT}.apk</PackagePath>\n" #e:\P4\CE_STREAMS2\bin\win_x86\AndroidLauncher.apk
		"<DebuggerFlavor>AndroidDebugger</DebuggerFlavor>\n"
		"<AdditionalSymbolSearchPaths></AdditionalSymbolSearchPaths>\n" #e:\P4\CE_STREAMS2\bin\win_x86\lib_debug\armeabi-v7a;$(AdditionalSymbolSearchPaths)
	"</PropertyGroup>\n"
		)
	endforeach()
		
	file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${THIS_PROJECT}.Packaging.androidproj.user	
	"</Project>"
	)


endmacro()
