start_sources()

# -- cry3dengineunittest.waf_files --
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"ParticleSystemUnitTests.cpp"
)
end_sources()

CryUnitTestSuite(Cry3DEngineUnitTest SOLUTION_FOLDER "UnitTests")
use_scaleform()
include_directories("${CRYENGINE_DIR}/Code/CryEngine/Cry3DEngine")
target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
if (NOT OPTION_STATIC_LINKING)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystemLib Cry3DEngineLib)
else()
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystem Cry3DEngine)
endif()

if (ORBIS OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-unknown-pragmas #See usages of raw #pragma warning
		-Wno-unused-private-field #See CScopedWireFrameMode
		-Wno-multichar #CryNetwork uses multichar constants such as 'bool'
		-Wno-unused-function #See ByteOrder, CryGetModuleFileName, WrappedF_Break
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-unused-function
		-Wno-unused-value
		-Wno-unused-variable
		-Wno-uninitialized
		-Wno-sign-compare
		-Wno-ignored-attributes
		-Wno-int-in-bool-context
		-Wno-unused-but-set-variable
		-Wno-misleading-indentation
		-Wno-strict-aliasing
		-Wno-maybe-uninitialized
		-Wno-unknown-pragmas
		-Wno-unused-const-variable
		-Wno-multichar
		-Wno-char-subscripts
		-Wno-parentheses
		-Wno-comment
		-Wno-address
		-Wno-format
		-Wno-switch
		-Wno-stringop-truncation
	)
elseif (MSVC)
	target_compile_options(${THIS_PROJECT} PRIVATE
		/we5038 #data member 'member1' will be initialized after data member 'member2' data member 'member' will be initialized after base class 'base_class'
	)
endif()
