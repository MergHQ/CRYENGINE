if (NOT NDKROOT)
	set(NDKROOT $ENV{NDKROOT})
endif()

file(TO_CMAKE_PATH "${NDKROOT}" NDKROOT)

string(REPLACE "\\" "/" NDKROOT ${NDKROOT})

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
	set(MSVC TRUE CACHE BOOL "Visual Studio Compiler" FORCE INTERNAL)
endif()

set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/android")

set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_SYSTEM "ANDROID")
set(CMAKE_SYSTEM_NAME "Android")
set(CMAKE_GENERATOR_TOOLSET gcc-4.8)

set(BUILD_PLATFORM "Android")
set(ANDROID TRUE)
set(CMAKE_SYSTEM_VERSION "1")
set(CMAKE_SYSTEM_PROCESSOR "ARM")

set(CMAKE_ANDROID_API 19)
set(CMAKE_ANDROID_API_MIN 17)
set(CMAKE_ANDROID_ARCH armv7-a)

set(CMAKE_CONFIGURATION_TYPES Debug Profile Release)
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)

include_directories(
	${NDKROOT}/platforms/android-${CMAKE_ANDROID_API}/arch-${CMAKE_ANDROID_ARCH}/usr/include 
	${NDKROOT}/sources/android/support/include
	${NDKROOT}/sources/android/native_app_glue
	${CMAKE_SOURCE_DIR}/Code/Tools/SDLExtension/src/include
)

set(OPTION_STATIC_LINKING TRUE)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_definitions(-D_DEBUG=1)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()
#-Werror

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=softfp -march=armv7-a -Wall  -ffast-math -Wno-char-subscripts -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-value -Wno-parentheses -Wno-switch -Wno-unused-function -Wno-multichar -Wno-format-security -Wno-empty-body -Wno-comment -Wno-char-subscripts -Wno-sign-compare -Wno-narrowing -Wno-write-strings -Wno-format -Wno-strict-aliasing -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-strict-overflow -Wno-uninitialized -Wno-unused-local-typedefs -Wno-deprecated -Wno-unused-result -Wno-sizeof-pointer-memaccess -Wno-array-bounds  -fno-rtti -fno-exceptions -Wno-invalid-offsetof -Wno-reorder -Wno-conversion-null -Wno-overloaded-virtual -Wno-c++0x-compat -std=c++11 -std=gnu++11 -DANDROID -DDISABLE_IMPORTGL -D__ANDROID__  -D__ARM_ARCH_7A__ -D_LINUX -DLINUX -DLINUX32 -DCRY_MOBILE -D_HAS_C9X -D_PROFILE -DPROFILE -D_MT -D_DLL")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon -mfloat-abi=softfp -march=armv7-a -Wall  -ffast-math -Wno-char-subscripts -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-value -Wno-parentheses -Wno-switch -Wno-unused-function -Wno-multichar -Wno-format-security -Wno-empty-body -Wno-comment -Wno-char-subscripts -Wno-sign-compare -Wno-narrowing -Wno-write-strings -Wno-format -Wno-strict-aliasing -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-strict-overflow -Wno-uninitialized -Wno-unused-local-typedefs -Wno-deprecated -Wno-unused-result -Wno-sizeof-pointer-memaccess -Wno-array-bounds -DANDROID -DDISABLE_IMPORTGL -D__ANDROID__  -D__ARM_ARCH_7A__ -D_LINUX -DLINUX -DLINUX32 -DCRY_MOBILE -D_HAS_C9X -D_PROFILE -DPROFILE -D_MT -D_DLL")

set(CMAKE_CXX_FLAGS_PROFILE "-O2" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_C_FLAGS_PROFILE "-O2" CACHE STRING "C Profile Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2" CACHE STRING "C++ Release Flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-O2" CACHE STRING "C Release Flags" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Library Profile Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Executable Profile Flags" FORCE)

set( CMAKE_SHARED_LINKER_FLAGS "-Wl,--whole-archive -Wl,-Bdynamic -Wl,--allow-multiple-definition" )
set( CMAKE_STATIC_LINKER_FLAGS "-Wl,--whole-archive -Wl,-Bstatic -Wl,--allow-multiple-definition" )
set( CMAKE_EXE_LINKER_FLAGS " -Wl,--allow-multiple-definition" )

#-mfpu=neon -mfloat-abi=softfp -march=armv7-a --sysroot=D:\NVPACK\android-ndk-r10c/platforms/android-19/arch-arm -Wall -Werror -ffast-math -Wno-char-subscripts -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-value -Wno-parentheses -Wno-switch -Wno-unused-function -Wno-multichar -Wno-format-security -Wno-empty-body -Wno-comment -Wno-char-subscripts -Wno-sign-compare -Wno-narrowing -Wno-write-strings -Wno-format -Wno-strict-aliasing -Wno-unused-but-set-variable -Wno-maybe-uninitialized -Wno-strict-overflow -Wno-uninitialized -Wno-unused-local-typedefs -Wno-deprecated -Wno-unused-result -Wno-sizeof-pointer-memaccess -Wno-array-bounds -fno-rtti -fno-exceptions -Wno-invalid-offsetof -Wno-reorder -Wno-conversion-null -Wno-overloaded-virtual -Wno-c++0x-compat -std=c++11 -std=gnu++11 -O2 

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
		"				<meta-data android:name=\"android.app.lib_name\" android:value=\"AndroidLauncher\" />\n"
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
		"	<project name=\"AndroidLauncher\" default=\"help\">\n"
		"\n"
		"		<property file=\"build.properties\" />\n"
		"\n"
		"		<!--REGION-BEGIN: The following properties allow Nsight Tegra to maintain build results per-configuration (debug, release, etc)-->\n"
		"		<target name=\"-pre-compile\">\n"
		"			<property name=\"nsight.tegra.project.all.jars.path\" value=\"\${toString:project.all.jars.path}\" />\n"
		"			<path id=\"project.all.jars.path\">\n"
		"				<path path=\"\${nsight.tegra.project.all.jars.path}\" />\n"
		"				<fileset dir=\"\${jar.libs.dir}\">\n"
		"					<include name=\"*.jar\" />\n"
		"				</fileset>\n"
		"			</path>\n"
		"			<echo>Ant Java version: \${ant.java.version}</echo>\n"
		"			<echo>Java Tools version: \${java.version}</echo>\n"
		"		</target>\n"
		"		<!--REGION-END-->\n"
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
	file(COPY ${NDKROOT}/sources/cxx-stl/gnu-libstdc++/4.8/libs/armeabi-v7a/libgnustl_shared.so DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${NDKROOT}/prebuilt/android-arm/gdbserver/gdbserver DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${CMAKE_SOURCE_DIR}/Code/Tools/SDLExtension/lib/android-armeabi-v7a/libSDL2Ext.so DESTINATION ${apk_folder}/lib/armeabi-v7a)
	file(COPY ${SDK_DIR}/SDL2/lib/android-armeabi-v7a/libSDL2.so DESTINATION ${apk_folder}/lib/armeabi-v7a)

	#Make ANT run
	file(TO_NATIVE_PATH "${OUTPUT_DIRECTORY}" NATIVE_OUTDIR)
	file(TO_NATIVE_PATH "${apk_folder}" apk_folder_native)

	add_custom_command(TARGET ${THIS_PROJECT} POST_BUILD COMMAND copy ${NATIVE_OUTDIR}\\stripped_libAndroidLauncher.so ${apk_folder_native}\\lib\\armeabi-v7a\\libAndroidLauncher.so COMMAND call $ENV{ANT_HOME}/bin/ant clean COMMAND call $ENV{ANT_HOME}/bin/ant debug COMMAND copy ${apk_folder_native}\\bin\\AndroidLauncher-debug.apk ${NATIVE_OUTDIR}\\AndroidLauncher.apk WORKING_DIRECTORY ${apk_folder})	
endmacro()
