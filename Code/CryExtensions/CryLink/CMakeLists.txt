option(EXTENSION_CRYLINK "Enable CryLink" ON)
if (NOT EXTENSION_CRYLINK)
	return()
endif()

add_subdirectory(Module)

if (OPTION_SANDBOX)
	# TODO: is editor plugin needed?
	# add_subdirectory(EditorPlugin)
endif()
