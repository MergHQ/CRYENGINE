if ((WINDOWS AND EXISTS "${SDK_DIR}/discord-rpc") AND OPTION_ENGINE)
	option(PLUGIN_GAMEPLATFORM_DISCORD "Enable Discord Rich Presence integration plug-in" ON)
else()
	option(PLUGIN_GAMEPLATFORM_DISCORD "Enable Discord Rich Presence integration plug-in" OFF)
endif()
	
if (NOT PLUGIN_GAMEPLATFORM_DISCORD)
	return()
endif()

start_sources()

# -- discord.waf_files --
sources_platform(WINDOWS)
add_sources("CryGamePlatform_Discord_uber_0.cpp"
	SOURCE_GROUP "Root"
		"resource.h"
		"StdAfx.cpp"
		"StdAfx.h"
)

add_sources("CryGamePlatform_Discord_uber_1.cpp"
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

add_sources("CryGamePlatform_Discord_uber_2.cpp"
	SOURCE_GROUP "Root"
		"DiscordService.cpp"
		"DiscordService.h"
		"DiscordTypes.h"
	SOURCE_GROUP "Profile"
		"DiscordUserIdentifier.h"
		"DiscordAccount.cpp"
		"DiscordAccount.h"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
)

end_sources()

CryEngineModule(CryGamePlatformDiscord PCH "StdAfx.cpp" SOLUTION_FOLDER "CryPlugins")

target_include_directories(${THIS_PROJECT}
PRIVATE 
	"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
PUBLIC
	../../Interface
	../../Common
)

if(WINDOWS)
	include(${TOOLS_CMAKE_DIR}/modules/discord-rpc.cmake)

	target_link_libraries(${THIS_PROJECT} PRIVATE discord-rpc)
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
