start_sources()

sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"All.cpp"
)
end_sources()

CryEditorPluginUnitTestSuite(SamplePluginUnitTest SOLUTION_FOLDER "UnitTests")
target_include_directories(${THIS_PROJECT} PRIVATE "${CRYENGINE_DIR}/Code/Sandbox/Plugins/SamplePlugin")
target_link_libraries(${THIS_PROJECT} PRIVATE SamplePlugin delayimp) #delayimp is required for /DELAYLOAD

#DELAYLOAD is essential for loading dll from another folder, see also EDITOR_PLUGIN_UNIT_TEST
set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY LINK_FLAGS " /DELAYLOAD:SamplePlugin.dll") 