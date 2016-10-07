if(OPTION_BUILD_CSHARP_WITH_MCS)
	project(CryEngine.Tests NONE)
	
	set(SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../SDKs)
	set(MCS_PATH ${SDK_DIR}/Mono/bin/mcs.bat)
	set(MONO_LIB_PATH ${SDK_DIR}/Mono/lib/mono)

	set(CRYMONOBRIDGE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../bin/win_x64)

	set(Serialization_FILES Serialization/DomainSerializerTests.cs)
	source_group("Serialization" FILES ${Serialization_FILES})

	set(SOURCE ${Serialization_FILES})

	add_custom_target(CryEngine.Core.Tests
		COMMAND ${MCS_PATH} -target:library -langversion:6 -platform:anycpu -optimize -debug -define:WIN64 -lib:${MONO_LIB_PATH} -r:System -r:System.Data -r:System.Drawing -r:System.Runtime.Serialization -r:System.Windows.Forms -r:System.Xml -r ${SDK_DIR}/nunit/framework/3.4.1.0/net-4.5/nunit.framework.dll -r:${CRYMONOBRIDGE_PATH}/CryEngine.Core.dll ${SOURCE} -out:${CRYMONOBRIDGE_PATH}/CryEngine.Core.Tests.dll
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		SOURCES ${SOURCE})
else()
	include_external_msproject(CryEngine.Core.Tests ${CMAKE_CURRENT_LIST_DIR}/Core.Tests.csproj)
endif()

add_dependencies(CryEngine.Core.Tests CryEngine.Core)

set_property(TARGET CryEngine.Core.Tests PROPERTY FOLDER "CryMono")