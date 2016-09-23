set(CLANG_COMMON_FLAGS 
	-Werror
	-Wall
	
	-Wno-overloaded-virtual
	-Wno-unknown-pragmas
	-Wno-unused-variable
	-Wno-unused-value
	-Wno-tautological-compare
	-Wno-reorder
	-Wno-parentheses
	-Wno-switch
	-Wno-deprecated-writable-strings
	-Wno-unneeded-internal-declaration
	-Wno-unused-function
	-Wno-multichar
	-Wno-format-security
	-Wno-self-assign
	-Wno-c++11-narrowing
	-Wno-empty-body
	-Wno-comment
	-Wno-dynamic-class-memaccess
	-Wno-sizeof-pointer-memaccess
	-Wno-char-subscripts
	-Wno-invalid-offsetof
	-Wno-unused-private-field
	-Wno-format
	-Wno-writable-strings
	-Wno-null-conversion
	-Wno-sometimes-uninitialized
	-Wno-logical-op-parentheses
	-Wno-shift-negative-value
	
	-D_HAS_C9X
	
	-gfull
	
	-ffunction-sections
	-fdata-sections
	-funwind-tables
	-fno-omit-frame-pointer
	-fno-strict-aliasing	
	)	
	
string(REPLACE ";" " " CLANG_COMMON_FLAGS "${CLANG_COMMON_FLAGS}")
	
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_COMMON_FLAGS} -ffast-math -fno-rtti" CACHE STRING "C++ Common Flags" FORCE)
 # Set MSVC option for Visual Studio properties to be displayed correctly
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -D_DEBUG" CACHE STRING "C++ Debug Flags" FORCE)
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -D_RELEASE" CACHE STRING "C++ Release Flags" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Libraries linked by defalut with all C++ applications." FORCE)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_COMMON_FLAGS}" CACHE STRING "C Common Flags" FORCE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -D_DEBUG" CACHE STRING "C Debug Flags" FORCE)
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -D_RELEASE" CACHE STRING "C Release Flags" FORCE)

set(CMAKE_LINK_FLAGS "{$CMAKE_LINK_FLAGS}") #-Wl,-dead_strip

set(CMAKE_SHARED_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Library Profile Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Executable Profile Flags" FORCE)