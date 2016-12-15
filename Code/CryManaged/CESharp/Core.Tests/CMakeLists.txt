if(OPTION_BUILD_CSHARP_WITH_MCS)
	project(CryEngine.Tests NONE)
	
	set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../SDKs)
	set(MCS_PATH ${SDK_DIR}/Mono/bin/mcs.bat)
	if(MONO_WIN64)
		set(CRYMONOBRIDGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/win_x64)
	else() 
		set(CRYMONOBRIDGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/win_x86)
	endif()
	
	create_mono_compiler_settings()
	set(Serialization_FILES Serialization/DomainSerializerTests.cs)
	source_group("Serialization" FILES ${Serialization_FILES})

	set(SOURCE ${Serialization_FILES})

	add_custom_target(CryEngine.Core.Tests
		COMMAND ${MCS_PATH} -target:library -langversion:${MONO_LANGUAGE_VERSION} -platform:${MONO_CPU_PLATFORM} -optimize -debug -define:${MONO_PREPROCESSOR_DEFINE} -lib:${MONO_LIB_PATH} -r:System -r:System.Data -r:System.Drawing -r:System.Runtime.Serialization -r:System.Windows.Forms -r:System.Xml -r ${SDK_DIR}/nunit/framework/3.4.1.0/net-4.5/nunit.framework.dll -r:${CRYMONOBRIDGE_PATH}/CryEngine.Core.dll ${SOURCE} -out:${CRYMONOBRIDGE_PATH}/CryEngine.Core.Tests.dll
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		SOURCES ${SOURCE})
else()
	include_external_msproject(CryEngine.Core.Tests ${CMAKE_CURRENT_LIST_DIR}/Core.Tests.csproj)
endif()

add_dependencies(CryEngine.Core.Tests CryEngine.Core)

set_property(TARGET CryEngine.Core.Tests PROPERTY FOLDER "CryMono")