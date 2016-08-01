cmake_minimum_required (VERSION 3.6.0)
project (Game VERSION 1.0 LANGUAGES CXX)

if(MSVC)
	configure_file ("${PROJECT_NAME}.vcxproj.user.in" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user")
endif(MSVC)

find_package (CryCommon REQUIRED)
find_package (CryAction REQUIRED)

#START-FILE-LIST

set (OPTION_PCH ON CACHE BOOL "Enable Precompiled headers")

set (SourceGroup_Dll
	"Main.cpp"
	"StdAfx.cpp"
	"StdAfx.h"
)
source_group("Dll" FILES ${SourceGroup_Dll})

set (SourceGroup_Editor
	"editor/EditorGame.cpp"
	"editor/EditorGame.h"
)
source_group("Editor" FILES ${SourceGroup_Editor})

set (SourceGroup_Entities
	"entities/GeomEntity.cpp"
	"entities/GeomEntity.h"
)
source_group("Entities" FILES ${SourceGroup_Entities})

set (SourceGroup_Flownodes
	"flownodes/FlowBaseNode.h"
	"flownodes/FlowGameEntityNode.cpp"
	"flownodes/FlowGameEntityNode.h"
)
source_group("Flownodes" FILES ${SourceGroup_Flownodes})

set (SourceGroup_Game
	"game/Game.cpp"
	"game/Game.h"
	"game/GameFactory.cpp"
	"game/GameFactory.h"
	"game/GameRules.cpp"
	"game/GameRules.h"
)
source_group("Game" FILES ${SourceGroup_Game})

set (SourceGroup_Player
	"player/Player.cpp"
	"player/Player.h"
)
source_group("Player" FILES ${SourceGroup_Player})

set (SourceGroup_PlayerExtensions
	"player/extensions/InputExtension.cpp"
	"player/extensions/InputExtension.h"
	"player/extensions/ISimpleExtension.h"
	"player/extensions/MovementExtension.cpp"
	"player/extensions/MovementExtension.h"
	"player/extensions/ViewExtension.cpp"
	"player/extensions/ViewExtension.h"
)
source_group("Player\\Extensions" FILES ${SourceGroup_PlayerExtensions})

set (SourceGroup_Startup
	"startup/GameStartup.cpp"
	"startup/GameStartup.h"
)
source_group("Startup" FILES ${SourceGroup_Startup})

set (SOURCE
	${SourceGroup_Dll}
	${SourceGroup_Editor}
	${SourceGroup_Entities}
	${SourceGroup_Flownodes}
	${SourceGroup_Game}
	${SourceGroup_Player}
	${SourceGroup_PlayerExtensions}
	${SourceGroup_Startup}	
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
