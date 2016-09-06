set(XBUILD_EXE ${SDK_DIR}/Mono/bin/xbuild.bat)

macro(parse_cs_files OUTPUT_VAR FILENAME)
	file(READ ${FILENAME} CSPROJ_CONTENT) 
	string(REGEX MATCHALL "\"[0-9a-zA-Z\\_.]+\\.cs\"" CS_FILES "${CSPROJ_CONTENT}")
	string(REPLACE "\"" "" NO_QUOTES "${CS_FILES}")
	string(REPLACE "\\" "/" ${OUTPUT_VAR} "${NO_QUOTES}")
endmacro()

macro(parse_csproj_files OUTPUT_VAR FILENAME)
	file(READ ${FILENAME} CSPROJ_CONTENT) 
	string(REGEX MATCHALL "\"[0-9a-zA-Z\\_.]+\\.csproj\"" CSPROJ_REFS "${CSPROJ_CONTENT}")
	string(REPLACE "\"" "" NO_QUOTES "${CSPROJ_REFS}")
	string(REPLACE "\\" "/" ${OUTPUT_VAR} "${NO_QUOTES}")
endmacro()

macro(parse_property OUTPUT_VAR FILENAME PROPERTY_NAME)
	file(READ ${FILENAME} CSPROJ_CONTENT) 
	string(REGEX MATCH "<${PROPERTY_NAME}>.*</${PROPERTY_NAME}>" XML_STRING ${CSPROJ_CONTENT})
	string(REGEX REPLACE "</?${PROPERTY_NAME}>" "" ${OUTPUT_VAR} ${XML_STRING})
endmacro()


function(add_csproj CSPROJ_FILE) 
	parse_cs_files(CS_FILES ${CSPROJ_FILE})
	parse_csproj_files(REFERENCES ${CSPROJ_FILE})
	get_filename_component(PROJ_PATH ${CSPROJ_FILE} DIRECTORY)

	parse_property(ASSEMBLY_NAME ${CSPROJ_FILE} "AssemblyName")
	
	# TODO: Figure out output path, or override it with /p:OutputPath
	list(APPEND ASSEMBLIES ${CMAKE_SOURCE_DIR}/bin/win_x64/${ASSEMBLY_NAME}.dll)

	# Find all .cs source files used in the project or any of it's referneced projects
	foreach(CS_FILE ${CS_FILES})
		list(APPEND CS_PATHS ${PROJ_PATH}/${CS_FILE})
	endforeach()
	foreach(REFERENCE ${REFERENCES})
		get_filename_component(reference_path ${PROJ_PATH}/${REFERENCE} DIRECTORY)
		parse_cs_files(cs_files ${PROJ_PATH}/${REFERENCE})
		foreach(cs_file ${cs_files})
			list(APPEND CS_PATHS ${reference_path}/${cs_file})
		endforeach()
	endforeach()

	add_custom_command(OUTPUT ${ASSEMBLIES}
		COMMAND ${XBUILD_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/${CSPROJ_FILE} /p:Platform=x64 /v:q /nologo
		COMMENT "Building Mono project \"${CSPROJ_FILE}\""
		DEPENDS CryMonoBridge ${CS_PATHS} ${CSPROJ_FILE})

	add_custom_target(${ASSEMBLY_NAME} DEPENDS ${ASSEMBLIES})
endfunction()
