cmake_minimum_required (VERSION 3.6.2)

#Validations of the Input parameters
if (NOT DEFINED CRYENGINE_DIR)
	message( FATAL_ERROR "CRYENGINE_DIR variable must be defined and point to a valid path to CryEngine root folder." )
endif()
if (NOT DEFINED PROJECT_FILE)
	message( FATAL_ERROR "PROJECT_FILE variable must be defined and point to a valid .cryproject file." )
endif()

# Include Common CMake macros used by CRYENGINE cmake files
include(${CRYENGINE_DIR}/Tools/CMake/Project.cmake)

project (Game VERSION 1.0 LANGUAGES CXX)

set (SourceGroup_PluginMain
	"StdAfx.cpp"
	"StdAfx.h"
	"GamePlugin.cpp"
	"GamePlugin.h"
)
source_group("Main" FILES ${SourceGroup_PluginMain})

set (SourceGroup_Game
	"Game/GameRules.cpp"
	"Game/GameRules.h"
)
source_group("Game" FILES ${SourceGroup_Game})

set (SourceGroup_WeaponEntities
	"Entities/Gameplay/Weapons/Bullet.cpp"
	"Entities/Gameplay/Weapons/Bullet.h"
	"Entities/Gameplay/Weapons/ISimpleWeapon.h"
	"Entities/Gameplay/Weapons/Rifle.cpp"
	"Entities/Gameplay/Weapons/Rifle.h"
)
source_group("Entities\\Gameplay\\Weapons" FILES ${SourceGroup_WeaponEntities})

set (SourceGroup_GameplayEntities
	"Entities/Gameplay/SpawnPoint.cpp"
	"Entities/Gameplay/SpawnPoint.h"
)
source_group("Entities\\Gameplay" FILES ${SourceGroup_GameplayEntities})

set (SourceGroup_EntityHelpers
	"Entities/Helpers/ISimpleExtension.h"
)
source_group("Entities\\Helpers" FILES ${SourceGroup_EntityHelpers})

set (SourceGroup_Player
	"Player/Player.cpp"
	"Player/Player.h"
	"Player/ISimpleActor.h"
)
source_group("Player" FILES ${SourceGroup_Player})

set (SourceGroup_PlayerAnimations
	"Player/Animations/PlayerAnimations.cpp"
	"Player/Animations/PlayerAnimations.h"
)
source_group("Player\\Animations" FILES ${SourceGroup_PlayerAnimations})

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

set (SourceGroup_PlayerPathfinding
	"Player/Pathfinding/PlayerPathfinding.cpp"
	"Player/Pathfinding/PlayerPathfinding.h"
)
source_group("Player\\Pathfinding" FILES ${SourceGroup_PlayerPathfinding})

set (SOURCE
	${SourceGroup_PluginMain}
	${SourceGroup_Game}
	${SourceGroup_WeaponEntities}
	${SourceGroup_GameplayEntities}
	${SourceGroup_EntityHelpers}
	${SourceGroup_Player}
	${SourceGroup_PlayerAnimations}
	${SourceGroup_PlayerInput}
	${SourceGroup_PlayerMovement}
	${SourceGroup_PlayerView}
	${SourceGroup_PlayerPathfinding}
)


add_library(${PROJECT_NAME} MODULE ${SOURCE})
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE Ntdll)
endif(WIN32)


# Set Visual Studio solution project name
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Project")
# Set StartUp project in Visual Studio
set_solution_startup_target(${PROJECT_NAME} )

#Enable Precompiled Headers
USE_MSVC_PRECOMPILED_HEADER(${PROJECT_NAME} "StdAfx.h" "StdAfx.cpp")

if (WIN32 AND DEFINED PROJECT_FILE)
	set_visual_studio_debugger_command( ${PROJECT_NAME} "${CRYENGINE_DIR}\\bin\\win_x64\\GameLauncher.exe" "-project \"${PROJECT_FILE}\"" )
endif()
