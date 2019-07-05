add_library(Python SHARED IMPORTED GLOBAL)

target_include_directories(Python INTERFACE "${SDK_DIR}/Python/include")

if(WINDOWS)
	set_property(TARGET Python APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG PROFILE)

	set(PYTHON_LIBS "${SDK_DIR}/Python/x64/libs")

	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_DEBUG ${PYTHON_LIBS}/python3_d.lib)
	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_DEBUG ${PYTHON_LIBS}/python37_d.lib)
	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_PROFILE ${PYTHON_LIBS}/python3.lib)
	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_PROFILE ${PYTHON_LIBS}/python37.lib)
	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_RELEASE ${PYTHON_LIBS}/python3.lib)
	set_target_properties(Python PROPERTIES	IMPORTED_IMPLIB_RELEASE ${PYTHON_LIBS}/python37.lib)

	#Python runtime is needed only for Sandbox: Debug and Profile configurations. Setting second arg to "" will not deploy the files to Release folder
	deploy_runtime_files(${SDK_DIR}/Python/x64/bin/python3.dll "")
	deploy_runtime_files(${SDK_DIR}/Python/x64/bin/python3_d.dll "")
	deploy_runtime_files(${SDK_DIR}/Python/x64/bin/python37.dll "")
	deploy_runtime_files(${SDK_DIR}/Python/x64/bin/python37_d.dll "")
	deploy_runtime_files(${SDK_DIR}/Python/x64/python37.zip "")
	deploy_runtime_files(${SDK_DIR}/Python/x64/python37_d.zip "")

else()
	message(FATAL_ERROR "You need to compile Python from source for this platform. Use ${SDK_DIR}/Python/src ")
endif()
