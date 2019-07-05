# -- python.waf_files --

start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Root"
	"main.cpp"
)
end_sources()

if(WINDOWS)
	CryConsoleApplication(sandboxpython SOLUTION_FOLDER "Sandbox/Libs")
	target_compile_definitions(${THIS_PROJECT} PRIVATE -DPy_BUILD_CORE -DWIN32 $<$<CONFIG:Debug>:_DEBUG>)
	target_link_libraries(${THIS_PROJECT} PRIVATE Shlwapi)
	target_link_libraries(${THIS_PROJECT} PRIVATE ${SDK_DIR}/Python/x64/libs/python3$<$<CONFIG:Debug>:_d>.lib)
	target_link_libraries(${THIS_PROJECT} PRIVATE ${SDK_DIR}/Python/x64/libs/python37$<$<CONFIG:Debug>:_d>.lib)
	target_include_directories(${THIS_PROJECT} PRIVATE "${SDK_DIR}/Python/include")
else()
	CryFileContainer(sandboxpython SOLUTION_FOLDER "Libs")
endif()


