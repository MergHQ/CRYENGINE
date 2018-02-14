start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"TestFrameworkTest.cpp" 
		"StringTests.cpp"
		"ArrayTests.cpp"
		"PathTests.cpp"
)
end_sources()

CryUnitTestSuite(CryCommonUnitTest SOLUTION_FOLDER "UnitTests")
target_compile_definitions(${THIS_PROJECT} PRIVATE NOT_USE_CRY_MEMORY_MANAGER)
