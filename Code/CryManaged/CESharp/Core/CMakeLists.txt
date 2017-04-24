if(OPTION_BUILD_CSHARP_WITH_MCS)
	project(CESharp NONE)

	set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../SDKs)
	set(MCS_PATH ${SDK_DIR}/Mono/bin/mcs.bat)
	create_mono_compiler_settings()
	
	set(AI_FILES AI/Attributes.cs AI/BehaviorTreeNode.cs AI/BehaviorTreeNodeFactory.cs AI/LastingMovementRequest.cs)
	source_group("AI" FILES ${AI_FILES})

	set(Audio_FILES Audio/AudioManager.cs Audio/Native/NativeAudioMethods.cs Audio/AudioObject.cs Audio/AudioFile.cs Audio/AudioRequestInfo.cs Audio/Audio.cs)
	source_group("Audio" FILES ${Audio_FILES})


	set(Console_FILES Console/Attributes.cs Console/ConsoleCommand.cs Console/ConsoleVariable.cs Console/Native/NativeConsoleMethods.cs)
	source_group("Console" FILES ${Console_FILES})


	set(Debugging_FILES Debugging/DebugDraw.cs Debugging/ExceptionHandler.cs Debugging/ExceptionMessage.cs Debugging/ExceptionMessage.Designer.cs Debugging/Log.cs)
	source_group("Debugging" FILES ${Debugging_FILES})


	set(EntitySystem_FILES EntitySystem/Attributes.cs EntitySystem/Entity.cs EntitySystem/EntityComponent.cs EntitySystem/EntityId.cs EntitySystem/EntityPhysics.cs EntitySystem/EntityProperty.cs EntitySystem/EntityStatics.cs EntitySystem/Native/NativeEntityMethods.cs)
	source_group("EntitySystem" FILES ${EntitySystem_FILES})


	set(FlowSystem_FILES FlowSystem/FlowNode.cs FlowSystem/RunScene.cs)
	source_group("FlowSystem" FILES ${FlowSystem_FILES})


	set(Input_FILES Input/Input.cs Input/Mouse.cs Input/ActionHandler.cs)
	source_group("Input" FILES ${Input_FILES})


	set(Math_FILES Math/Angles3.cs Math/Color.cs Math/MathHelpers.cs Math/Matrix3x3.cs Math/Matrix3x4.cs Math/Matrix4x4.cs Math/Primitives.cs Math/Quaternion.cs Math/Vector2.cs Math/Vector3.cs Math/Vector4.cs)
	source_group("Math" FILES ${Math_FILES})


	set(Physics_FILES Physics/Ray.cs Physics/RaycastHit.cs)
	source_group("Physics" FILES ${Physics_FILES})


	set(Rendering_FILES Rendering/Renderer.cs)
	source_group("Rendering" FILES ${Rendering_FILES})


	set(Serialization_FILES Serialization/CachedTypeInfo.cs Serialization/DomainSerializer.cs Serialization/ObjectReader.cs Serialization/ObjectWriter.cs)
	source_group("Serialization" FILES ${Serialization_FILES})


	set(Util_FILES Util/Event.cs Util/FileSystem.cs Util/ReflectionHelper.cs Util/Tools.cs Util/IconType.cs Util/Attribute.cs)
	source_group("Util" FILES ${Util_FILES})


	set(Core_FILES Attributes.cs Camera.cs Environment.cs Extensions.cs GameFramework.cs ICryEnginePlugin.cs LevelSystem.cs Random.cs SystemEventHandler.cs Timer.cs)
	source_group("Core" FILES ${Core_FILES})

	set(SOURCE ${AI_FILES} ${Audio_FILES} ${Console_FILES} ${Debugging_FILES} ${EntitySystem_FILES} ${FlowSystem_FILES} ${Input_FILES} ${Math_FILES} ${TempPE_FILES} ${Debug_FILES} ${x64_FILES} ${TempPE_FILES} ${Debug_FILES} ${x86_FILES} ${obj_FILES} ${Physics_FILES} ${Rendering_FILES} ${Serialization_FILES} ${Util_FILES} ${Core_FILES})

	add_custom_target(CryEngine.Core ALL
		COMMAND ${MCS_PATH} -target:library -langversion:${MONO_LANGUAGE_VERSION} -platform:${MONO_CPU_PLATFORM} -optimize -debug -define:${MONO_PREPROCESSOR_DEFINE} -lib:${MONO_LIB_PATH} -r:System -r:System.Data -r:System.Drawing -r:System.Runtime.Serialization -r:System.Windows.Forms -r:System.Xml -r:${OUTPUT_DIRECTORY}/CryEngine.Common.dll ${SOURCE} -out:${OUTPUT_DIRECTORY}/CryEngine.Core.dll
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		SOURCES ${SOURCE})
		
	#Doesn't seem to do anything...
	set_target_properties(CryEngine.Core PROPERTIES VS_DOTNET_REFERENCES "System;System.Data")
else()
	include_external_msproject(CryEngine.Core ${CMAKE_CURRENT_LIST_DIR}/Core.csproj)
endif()

if (OPTION_CRYMONO)
	add_dependencies(CryEngine.Core CryMonoBridge)
endif()

set_property(TARGET CryEngine.Core PROPERTY FOLDER "CryMono")
