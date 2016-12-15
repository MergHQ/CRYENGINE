cmake_minimum_required (VERSION 3.6.0)
project (Game VERSION 1.0 LANGUAGES CXX)

if (${CMAKE_GENERATOR} MATCHES "^Visual Studio")
	# Set startup project to launch Game.exe with this project
	configure_file ("Game.vcxproj.user.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

find_package (CryCommon REQUIRED)
find_package (CryAction REQUIRED)

option(OPTION_PCH "Enable Precompiled headers" ON)

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
)

if(OPTION_PCH AND MSVC)	
	set (PRECOMPILED_SOURCE ${SOURCE})
	list(REMOVE_ITEM PRECOMPILED_SOURCE "StdAfx.cpp")
	set (PRECOMPILED_BINARY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pch)
	set_source_files_properties (${PRECOMPILED_SOURCE} PROPERTIES COMPILE_FLAGS "/Yu\"StdAfx.h\" /Fp\"${PRECOMPILED_BINARY}\"" OBJECT_DEPENDS "${PRECOMPILED_BINARY}")
	set_source_files_properties (StdAfx.cpp PROPERTIES COMPILE_FLAGS "/Yc\"StdAfx.h\" /Fp\"${PRECOMPILED_BINARY}\"" OBJECT_OUTPUTS "${PRECOMPILED_BINARY}")
endif (OPTION_PCH AND MSVC)

add_library(${PROJECT_NAME} MODULE ${SOURCE})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE CryCommon CryAction)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE Ntdll)
endif(WIN32)
