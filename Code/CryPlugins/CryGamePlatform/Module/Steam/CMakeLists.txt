if (((WINDOWS OR LINUX OR APPLE) AND EXISTS "${SDK_DIR}/Steamworks") AND OPTION_ENGINE)
	option(PLUGIN_GAMEPLATFORM_STEAM "Enable Game platform integration plug-in for Steam" ON)
else()
	option(PLUGIN_GAMEPLATFORM_STEAM "Enable Game platform integration plug-in for Steam" OFF)
endif()
	
if (NOT PLUGIN_GAMEPLATFORM_STEAM)
	return()
endif()

start_sources()

# -- CryGamePlatform_Steam.waf_files --
sources_platform(ALL)
add_sources("CryGamePlatform_Steam_uber_0.cpp"
	SOURCE_GROUP "Root"
		"resource.h"
		"StdAfx.cpp"
		"StdAfx.h"
)

add_sources("CryGamePlatform_Steam_uber_1.cpp"
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

sources_platform(WINDOWS LINUX APPLE)
add_sources("CryGamePlatform_Steam_uber_2.cpp"
	SOURCE_GROUP "Root"
		"SteamService.cpp"
		"SteamService.h"
		"SteamTypes.h"
	SOURCE_GROUP "Profile"
		"SteamStatistics.cpp"
		"SteamStatistics.h"
		"SteamUserIdentifier.h"
		"SteamAccount.cpp"
		"SteamAccount.h"
		"SteamAchievement.cpp"
		"SteamAchievement.h"
		"SteamLeaderboards.cpp"
		"SteamLeaderboards.h"
	SOURCE_GROUP "Multiplayer"
		"SteamMatchmaking.cpp"
		"SteamMatchmaking.h"
		"SteamNetworking.cpp"
		"SteamNetworking.h"
		"SteamUserLobby.cpp"
		"SteamUserLobby.h"
		"SteamServer.cpp"
		"SteamServer.h"
	SOURCE_GROUP "RemoteStorage"
		"SteamRemoteFile.cpp"
		"SteamRemoteFile.h"
		"SteamRemoteStorage.cpp"
		"SteamRemoteStorage.h"
		"SteamSharedRemoteFile.cpp"
		"SteamSharedRemoteFile.h"
	SOURCE_GROUP "UserGeneratedContent"
		"UserGeneratedContent.cpp"
		"UserGeneratedContent.h"
		"UserGeneratedContentManager.cpp"
		"UserGeneratedContentManager.h"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
)

end_sources()

CryEngineModule(CryGamePlatformSteam PCH "StdAfx.cpp" SOLUTION_FOLDER "CryPlugins")

target_include_directories(${THIS_PROJECT}
PRIVATE 
	"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
PUBLIC
	../../Interface
	../../Common
)

if(WINDOWS OR LINUX OR APPLE)
	include(${TOOLS_CMAKE_DIR}/modules/Steamworks.cmake)

	target_link_libraries(${THIS_PROJECT} PRIVATE Steamworks)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-unused-variable
		-Wno-reorder
		-Wno-parentheses
		-Wno-unknown-pragmas
		-Wno-format
		-Wno-switch
		-Wno-multichar
		-Wno-unused-private-field
		-Wno-empty-body
		-Wno-unused-value
		-Wno-unused-function
	)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${THIS_PROJECT} PRIVATE
		-Wno-unused-function
		-Wno-unused-value
		-Wno-unused-variable
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
	)
endif()
