set(PROJECT_BOOTSTRAP "${CMAKE_SOURCE_DIR}/bootstrap.dat")

CryGameModule(${CRYPROJECT_NAME} FORCE_SHARED PCH "StdAfx.cpp" SOLUTION_FOLDER "Project" API_NAME GAME_API)

#GameSDK specific
target_include_directories(${THIS_PROJECT} PRIVATE 
	"${SDK_DIR}/boost"
	"${CRYENGINE_DIR}/Code/CryEngine/CrySchematyc/Core/Interface"
	GameDll
	"${CRYENGINE_DIR}/Code/CryEngine/CryAction" 
	"${CRYENGINE_DIR}/Code/CryPlugins/CryLobby/Interface"
	"${CRYENGINE_DIR}/Code/CryEngine/CryCommon/Crylobby"
	"${CRYENGINE_DIR}/Code/CryPlugins/CryPerceptionSystem/Interface"
	"${CRYENGINE_DIR}/Code/CryPlugins/CryDefaultEntities/Module"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
    target_compile_options(${THIS_PROJECT} PRIVATE
        -Wno-unused-variable
        -Wno-reorder
        -Wno-unknown-pragmas
        -Wno-parentheses
        -Wno-switch
        -Wno-format
        -Wno-dynamic-class-memaccess
        -Wno-unused-private-field
        -Wno-unused-value
        -Wno-invalid-offsetof
        -Wno-multichar
        -Wno-char-subscripts
        -Wno-null-conversion
        -Wno-empty-body
        -Wno-unused-lambda-capture
        -Wno-unused-function
        -Wno-tautological-constant-out-of-range-compare
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${THIS_PROJECT} PRIVATE
        -Wno-unused-function
        -Wno-unused-value
        -Wno-unused-variable
        -Wno-aligned-new
        -Wno-int-in-bool-context
        -Wno-invalid-offsetof
        -Wno-comment
        -Wno-sign-compare
        -Wno-unused-but-set-variable
        -Wno-maybe-uninitialized
        -Wno-misleading-indentation
        -Wno-unused-result
        -Wno-unknown-pragmas
        -Wno-multichar
        -Wno-strict-aliasing
        -Wno-char-subscripts
        -Wno-conversion-null
        -Wno-reorder
        -Wno-parentheses
        -Wno-format
        -Wno-switch
    )
endif()