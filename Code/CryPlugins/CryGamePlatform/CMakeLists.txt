option(PLUGIN_GAMEPLATFORM "Enable CryGamePlatform plugin?" ON)

if (PLUGIN_GAMEPLATFORM)
	add_subdirectory(Module/Core)
	add_subdirectory(Module/Steam)
	add_subdirectory(Module/PSN)
endif()
