
if(WIN32)
	if (NOT METADATA_COMPANY)
		set(METADATA_COMPANY "Crytek GmbH")
	endif()
	set(METADATA_COMPANY ${METADATA_COMPANY} CACHE STRING "Company name for executable metadata")

	if (NOT METADATA_COPYRIGHT)
		string(TIMESTAMP year "%Y")
		set(METADATA_COPYRIGHT "(C) ${year} ${METADATA_COMPANY}")
	endif()
	set(METADATA_COPYRIGHT ${METADATA_COPYRIGHT} CACHE STRING "Copyright string for executable metadata")	

	if (NOT VERSION)
		set(VERSION "1.0.0.0")
	endif()
	set(METADATA_VERSION ${VERSION} CACHE STRING "Version number for executable metadata" FORCE)
	string(REPLACE . , METADATA_VERSION_COMMA ${METADATA_VERSION})
endif(WIN32)


if (MSVC_VERSION EQUAL 1900) # Visual Studio 2015
	set(MSVC_LIB_PREFIX vc140)
elseif (MSVC_VERSION EQUAL 1800) # Visual Studio 2013
	set(MSVC_LIB_PREFIX vc120)
elseif (MSVC_VERSION EQUAL 1700) # Visual Studio 2012
	set(MSVC_LIB_PREFIX "vc110")
else()
	set(MSVC_LIB_PREFIX "")
endif()


include_directories("${CryEngine_DIR}/Code/CryEngine/CryCommon")
#include_directories("${CryEngine_DIR}/Code/CryEngine/CryAction")
include_directories("${CryEngine_DIR}/Code/Libs/yasli")
include_directories("${SDK_DIR}/yasli")
include_directories("${SDK_DIR}/boost")

#engine
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/Cry3DEngine" "${CMAKE_BINARY_DIR}/CryEngine/Cry3DEngine")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAction" "${CMAKE_BINARY_DIR}/CryEngine/CryAction")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAISystem" "${CMAKE_BINARY_DIR}/CryEngine/CryAISystem")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAnimation" "${CMAKE_BINARY_DIR}/CryEngine/CryAnimation")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAudioSystem/Common" "${CMAKE_BINARY_DIR}/CryEngine/CryAudioSystem/Common")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAudioSystem/implementations/CryAudioImplFmod" "${CMAKE_BINARY_DIR}/CryEngine/CryAudioSystem/implementations/CryAudioImplFmod")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAudioSystem/implementations/CryAudioImplPortAudio" "${CMAKE_BINARY_DIR}/CryEngine/CryAudioSystem/implementations/CryAudioImplPortAudio")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAudioSystem/implementations/CryAudioImplSDLMixer" "${CMAKE_BINARY_DIR}/CryEngine/CryAudioSystem/implementations/CryAudioImplSDLMixer")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryAudioSystem/implementations/CryAudioImplWwise" "${CMAKE_BINARY_DIR}/CryEngine/CryAudioSystem/implementations/CryAudioImplWwise")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryCommon" "${CMAKE_BINARY_DIR}/CryEngine/CryCommon")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryDynamicResponseSystem" "${CMAKE_BINARY_DIR}/CryEngine/CryDynamicResponseSystem")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryEntitySystem" "${CMAKE_BINARY_DIR}/CryEngine/CryEntitySystem")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryFont" "${CMAKE_BINARY_DIR}/CryEngine/CryFont")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryInput" "${CMAKE_BINARY_DIR}/CryEngine/CryInput")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryMovie" "${CMAKE_BINARY_DIR}/CryEngine/CryMovie")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryNetwork" "${CMAKE_BINARY_DIR}/CryEngine/CryNetwork")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryPhysics" "${CMAKE_BINARY_DIR}/CryEngine/CryPhysics")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CryScriptSystem" "${CMAKE_BINARY_DIR}/CryEngine/CryScriptSystem")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/CrySystem" "${CMAKE_BINARY_DIR}/CryEngine/CrySystem")
add_subdirectory ("${CryEngine_DIR}/Code/CryEngine/RenderDll/XRenderD3D9" "${CMAKE_BINARY_DIR}/CryEngine/RenderDll/XRenderD3D9")

#libs
add_subdirectory ("${CryEngine_DIR}/Code/Libs/bigdigits" "${CMAKE_BINARY_DIR}/Libs/bigdigits")
#add_subdirectory ("${CryEngine_DIR}/Code/Libs/curl" "${CMAKE_BINARY_DIR}/Libs/curl")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/expat" "${CMAKE_BINARY_DIR}/Libs/expat")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/freetype" "${CMAKE_BINARY_DIR}/Libs/freetype")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/jsmn" "${CMAKE_BINARY_DIR}/Libs/jsmn")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/lua" "${CMAKE_BINARY_DIR}/Libs/lua")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/lz4" "${CMAKE_BINARY_DIR}/Libs/lz4")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/lzma" "${CMAKE_BINARY_DIR}/Libs/lzma")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/lzss" "${CMAKE_BINARY_DIR}/Libs/lzss")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/md5" "${CMAKE_BINARY_DIR}/Libs/md5")
#add_subdirectory ("${CryEngine_DIR}/Code/Libs/strophe" "${CMAKE_BINARY_DIR}/Libs/strophe")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/tomcrypt" "${CMAKE_BINARY_DIR}/Libs/tomcrypt")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/yasli" "${CMAKE_BINARY_DIR}/Libs/yasli")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/zlib" "${CMAKE_BINARY_DIR}/Libs/zlib")

#extensions
#include_directories("${CryEngine_DIR}/Code/CryExtensions/CrySchematyc/Core/Interface")
add_subdirectory ("${CryEngine_DIR}/Code/CryExtensions/CryLink" "${CMAKE_BINARY_DIR}/CryExtensions/CryLink")
add_subdirectory ("${CryEngine_DIR}/Code/CryExtensions/CrySchematyc" "${CMAKE_BINARY_DIR}/CryExtensions/CrySchematyc")

#plugins
# Mandatory plugin, contains entities required by the engine
add_subdirectory ("${CryEngine_DIR}/Code/CryPlugins/CryDefaultEntities/Module" "${CMAKE_BINARY_DIR}/CryPlugins/CryDefaultEntities/Module")

#launcher
if (DURANGO)
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/DurangoLauncher" "${CMAKE_BINARY_DIR}/Launcher/DurangoLauncher")
elseif (ORBIS)
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/OrbisLauncher" "${CMAKE_BINARY_DIR}/Launcher/OrbisLauncher")
elseif (ANDROID)
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/AndroidLauncher" "${CMAKE_BINARY_DIR}/Launcher/AndroidLauncher")
elseif (LINUX)
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/LinuxLauncher" "${CMAKE_BINARY_DIR}/Launcher/LinuxLauncher")
elseif (WIN32)
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/DedicatedLauncher" "${CMAKE_BINARY_DIR}/Launcher/DedicatedLauncher")
	add_subdirectory("${CryEngine_DIR}/Code/Launcher/WindowsLauncher" "${CMAKE_BINARY_DIR}/Launcher/WindowsLauncher")
endif ()

