include_guard(GLOBAL)

if(WINDOWS)
	add_library(discord-rpc SHARED IMPORTED GLOBAL)
	set_target_properties(discord-rpc PROPERTIES IMPORTED_LOCATION "${SDK_DIR}/discord-rpc/win64-dynamic/bin/discord-rpc.dll")
	set_target_properties(discord-rpc PROPERTIES IMPORTED_IMPLIB "${SDK_DIR}/discord-rpc/win64-dynamic/lib/discord-rpc.lib")
	set_target_properties(discord-rpc PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SDK_DIR}/discord-rpc/win64-dynamic/include")

	deploy_runtime_files(${SDK_DIR}/discord-rpc/win64-dynamic/bin/discord-rpc.dll)
endif()
