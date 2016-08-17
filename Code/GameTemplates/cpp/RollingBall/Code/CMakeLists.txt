cmake_minimum_required (VERSION 3.6.0)
project (RollingBallTemplate VERSION 1.0 LANGUAGES CXX)

if (MSVC)
	configure_file ("${PROJECT_NAME}.vcxproj.user.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user")
endif(MSVC)

find_package (CryCommon REQUIRED)
find_package (CryAction REQUIRED)

#START-FILE-LIST

set (OPTION_PCH ON CACHE BOOL "Enable Precompiled headers")

set (SourceGroup_Dll
	"StdAfx.cpp"
	"StdAfx.h"
)
source_group("Dll" FILES ${SourceGroup_Dll})

set (SourceGroup_EditorSpecific
	"EditorSpecific/EditorGame.cpp"
	"EditorSpecific/EditorGame.h"
)
source_group("EditorSpecific" FILES ${SourceGroup_EditorSpecific})

set (SourceGroup_Game
	"Game/Game.cpp"
	"Game/Game.h"
	"Game/GameRules.cpp"
	"Game/GameRules.h"
	"Game/GameFactory.cpp"
	"Game/GameFactory.h"
)
source_group("Game" FILES ${SourceGroup_Game})

set (SourceGroup_Flownodes
	"FlowNodes/Helpers/FlowBaseNode.h"
	"FlowNodes/Helpers/FlowGameEntityNode.cpp"
	"FlowNodes/Helpers/FlowGameEntityNode.h"
)
source_group("Flownodes" FILES ${SourceGroup_Flownodes})

set (SourceGroup_Startup
	"Startup/GameStartup.cpp"
	"Startup/GameStartup.h"
)
source_group("Startup" FILES ${SourceGroup_Startup})

set (SourceGroup_GameplayEntities
	"Entities/Gameplay/SpawnPoint.cpp"
	"Entities/Gameplay/SpawnPoint.h"
)
source_group("Entities\\Gameplay" FILES ${SourceGroup_GameplayEntities})

set (SourceGroup_EntityHelpers
	"Entities/Helpers/ISimpleExtension.h"
	"Entities/Helpers/NativeEntityBase.cpp"
	"Entities/Helpers/NativeEntityBase.h"
	"Entities/Helpers/NativeEntityPropertyHandling.cpp"
	"Entities/Helpers/NativeEntityPropertyHandling.h"
)
source_group("Entities\\Helpers" FILES ${SourceGroup_EntityHelpers})

set (SourceGroup_LightEntities
	"Entities/Lights/LightEntity.cpp"
	"Entities/Lights/LightEntity.h"
	"Entities/Lights/EnvironmentProbe.cpp"
	"Entities/Lights/EnvironmentProbe.h"
)
source_group("Entities\\Lights" FILES ${SourceGroup_LightEntities})

set (SourceGroup_PhysicsEntities
	"Entities/Physics/RigidBody.cpp"
	"Entities/Physics/RigidBody.h"
)
source_group("Entities\\Physics" FILES ${SourceGroup_PhysicsEntities})

set (SourceGroup_Player
	"Player/Player.cpp"
	"Player/Player.h"
	"Player/ISimpleActor.h"
)
source_group("Player" FILES ${SourceGroup_Player})

set (SourceGroup_PlayerInput
	"Player/Input/PlayerInput.cpp"
	"Player/Input/PlayerInput.h"
)
source_group("Player\\Input" FILES ${SourceGroup_PlayerInput})

set (SourceGroup_PlayerMovement
	"Player/Movement/PlayerMovement.cpp"
	"Player/Movement/PlayerMovement.h"
)
source_group("Player\\Movement" FILES ${SourceGroup_PlayerMovement})

set (SourceGroup_PlayerView
	"Player/View/PlayerView.cpp"
	"Player/View/PlayerView.h"
)
source_group("Player\\View" FILES ${SourceGroup_PlayerView})

set (SOURCE
	${SourceGroup_Dll}
	${SourceGroup_EditorSpecific}
	${SourceGroup_Game}
	${SourceGroup_Flownodes}
	${SourceGroup_Startup}
	${SourceGroup_GameplayEntities}
	${SourceGroup_EntityHelpers}
	${SourceGroup_LightEntities}
	${SourceGroup_PhysicsEntities}
	${SourceGroup_Player}
	${SourceGroup_PlayerInput}
	${SourceGroup_PlayerMovement}
	${SourceGroup_PlayerView}
)

set (HEADERS ${SOURCE})
list(FILTER HEADERS INCLUDE REGEX "\\.h$")
list(FILTER SOURCE EXCLUDE REGEX "\\.h$")

if(OPTION_PCH AND MSVC)	
	set (PRECOMPILED_BINARY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch)
	set_source_files_properties (${SOURCE} PROPERTIES COMPILE_FLAGS "/Yu\"StdAfx.h\" /Fp\"${PRECOMPILED_BINARY}\"" OBJECT_DEPENDS "${PRECOMPILED_BINARY}")
	set_source_files_properties (StdAfx.cpp PROPERTIES COMPILE_FLAGS "/Yc\"StdAfx.h\" /Fp\"${PRECOMPILED_BINARY}\"" OBJECT_OUTPUTS "${PRECOMPILED_BINARY}")
endif (OPTION_PCH AND MSVC)

#END-FILE-LIST

add_library(${PROJECT_NAME} MODULE ${SOURCE} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE CryCommon CryAction)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE Ntdll)
endif(WIN32)
