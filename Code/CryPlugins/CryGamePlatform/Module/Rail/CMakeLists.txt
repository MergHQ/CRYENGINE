if (WIN32 AND EXISTS "${SDK_DIR}/RailPlatformSDK" AND OPTION_ENGINE)
	option(PLUGIN_GAMEPLATFORM_RAIL "Enable Rail Game platform plug-in for WeGame integration" ON)
else()
	option(PLUGIN_GAMEPLATFORM_RAIL "Enable Rail Game platform plug-in for WeGame integration" OFF)
endif()
	
if (NOT PLUGIN_GAMEPLATFORM_RAIL)
	return()
endif()

start_sources()

# -- CryGamePlatform_Rail.waf_files --
sources_platform(ALL)
add_sources("CryGamePlatform_Rail_uber_0.cpp"
	SOURCE_GROUP "Root"
		"resource.h"
		"StdAfx.cpp"
		"StdAfx.h"
)

add_sources("CryGamePlatform_Rail_uber_1.cpp"
	SOURCE_GROUP "Interface"
		"../../Interface/IGamePlatform.h"
		"../../Interface/IPlatformAccount.h"
		"../../Interface/IPlatformAchievement.h"
		"../../Interface/IPlatformBase.h"
		"../../Interface/IPlatformLeaderboards.h"
		"../../Interface/IPlatformLobby.h"
		"../../Interface/IPlatformMatchmaking.h"
		"../../Interface/IPlatformNetworking.h"
		"../../Interface/IPlatformRemoteStorage.h"
		"../../Interface/IPlatformServer.h"
		"../../Interface/IPlatformService.h"
		"../../Interface/IPlatformStatistics.h"
		"../../Interface/IPlatformUser.h"
		"../../Interface/IPlatformUserGeneratedContent.h"
		"../../Interface/PlatformTypes.h"
		"../../Interface/PlatformIdentifier.h"
		"../../Interface/PlatformUserIdentifier.h"
)

sources_platform(WIN32)
add_sources("CryGamePlatform_Rail_uber_2.cpp"
	SOURCE_GROUP "Root"
		"RailService.cpp"
		"RailService.h"
		"RailTypes.h"
		"RailHelper.cpp"
		"RailHelper.h"
	SOURCE_GROUP "Profile"
		"RailAccount.cpp"
		"RailAccount.h"
	SOURCE_GROUP "Multiplayer"
		"RailMatchmaking.cpp"
		"RailMatchmaking.h"
		"RailNetworking.cpp"
		"RailNetworking.h"
		"RailUserLobby.cpp"
		"RailUserLobby.h"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
)

end_sources()

CryEngineModule(CryGamePlatformRail PCH "StdAfx.cpp" SOLUTION_FOLDER "CryPlugins")

target_include_directories(${THIS_PROJECT}
PRIVATE 
	"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
	"${SDK_DIR}/RailPlatformSDK"
PUBLIC
	../../Interface
	../../Common
)

if(WIN32)
	include(${TOOLS_CMAKE_DIR}/modules/RailPlatformSDK.cmake)
endif()
