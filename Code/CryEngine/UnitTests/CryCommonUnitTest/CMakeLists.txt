start_sources()
sources_platform(ALL)
add_sources("NoUberFile"
	SOURCE_GROUP "Source Files"
		"TypeTraitsTests.cpp"
		"TestFrameworkTest.cpp" 
		"MathTests.cpp"
		"HeapAllocatorTests.cpp"
		"StringTests.cpp"
		"StringUtilsTests.cpp"
		"ArrayTests.cpp"
		"RingBufferTests.cpp"
		"VariantTests.cpp"
		"AlignedVectorTests.cpp"
		"SmartPtrTests.cpp"
		"PathTests.cpp"
		"GUIDTests.cpp"
)
end_sources()

CryUnitTestSuite(CryCommonUnitTest SOLUTION_FOLDER "UnitTests")
if (NOT OPTION_STATIC_LINKING)
	target_compile_definitions(${THIS_PROJECT} PRIVATE NOT_USE_CRY_MEMORY_MANAGER)
else()
	use_scaleform()
	target_compile_definitions(${THIS_PROJECT} PRIVATE _LIB CRY_IS_MONOLITHIC_BUILD _LAUNCHER)
	target_link_libraries(${THIS_PROJECT} PRIVATE CrySystem)
endif()
