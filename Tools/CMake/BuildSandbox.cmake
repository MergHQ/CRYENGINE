
set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)

#modules
include("${CryEngine_DIR}/Tools/CMake/modules/FbxSdk.cmake")

#---

add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Libs/CryQt" "${CMAKE_BINARY_DIR}/Sandbox/Libs/CryQt")

set(EDITOR_DIR "${CryEngine_DIR}/Code/Sandbox/EditorQt" )
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/EditorQt" "${CMAKE_BINARY_DIR}/Sandbox/EditorQt")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/3DConnexionPlugin" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/3DConnexionPlugin")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorConsole" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorConsole")
add_subdirectory("${CryEngine_DIR}/Code/CryExtensions/CryLink/EditorPlugin" "${CMAKE_BINARY_DIR}/CryExtensions/CryLink/EditorPlugin")
	
add_custom_target(GameAndTools DEPENDS ${GAME_TARGET} Sandbox)

add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorCommon" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorCommon")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/EditorInterface" "${CMAKE_BINARY_DIR}/Sandbox/EditorInterface")

add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/CryDesigner" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/CryDesigner")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAudioControlsEditor" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAudioControlsEditor")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAudioControlsEditor/common" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAudioControlsEditor/common")
if(AUDIO_FMOD)
	add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorFmod" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAudioControlsEditor/EditorFmod")
endif()
if(AUDIO_SDL_MIXER)
	add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorSDLMixer" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAudioControlsEditor/EditorSDLMixer")
endif()
if (AUDIO_WWISE)
	add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAudioControlsEditor/EditorWwise" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAudioControlsEditor/EditorWwise")
endif()
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorAnimation" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorAnimation")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorDynamicResponseSystem" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorDynamicResponseSystem")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorEnvironment" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorEnvironment")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorParticle" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorParticle")
if (PLUGIN_SCHEMATYC)
	add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorSchematyc" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorSchematyc")
endif()
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/EditorTrackView" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/EditorTrackView")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/FbxPlugin" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/FbxPlugin")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/MeshImporter" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/MeshImporter")
add_subdirectory("${CryEngine_DIR}/Code/Sandbox/Plugins/PerforcePlugin" "${CMAKE_BINARY_DIR}/Sandbox/Plugins/PerforcePlugin")

#libs
add_subdirectory ("${CryEngine_DIR}/Code/Libs/prt" "${CMAKE_BINARY_DIR}/Libs/prt")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/python" "${CMAKE_BINARY_DIR}/Libs/python")
add_subdirectory ("${CryEngine_DIR}/Code/Libs/tiff" "${CMAKE_BINARY_DIR}/Libs/tiff")
