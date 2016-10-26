set(CLANG_COMMON_FLAGS
	-Wall
	-Werror

	-Wno-c++11-extensions
	-Wno-c++11-narrowing
	-Wno-char-subscripts
	-Wno-comment
	-Wno-conversion-null
	-Wno-deprecated-writable-strings
	-Wno-dynamic-class-memaccess
	-Wno-empty-body
	-Wno-format
	-Wno-format-security
	-Wno-invalid-offsetof
	-Wno-multichar
	-Wno-narrowing
	-Wno-null-character
	-Wno-overloaded-virtual
	-Wno-parentheses
	-Wno-reorder
	-Wno-self-assign
	-Wno-sign-compare
	-Wno-sizeof-pointer-memaccess
	-Wno-sometimes-uninitialized
	-Wno-switch
	-Wno-tautological-compare
	-Wno-unknown-pragmas
	-Wno-unneeded-internal-declaration
	-Wno-unused-function
	-Wno-unused-private-field
	-Wno-unused-result
	-Wno-unused-value
	-Wno-unused-variable

	-fvisibility-inlines-hidden
	-fvisibility=hidden
	-fPIC

	-msse2

	-D_HAS_C9X
	-Wno-write-strings
	-Wno-logical-op-parentheses
	-Wno-null-conversion
	-Wno-overloaded-virtual
	-Wno-shift-sign-overflow
	-Wno-shift-negative-value
	-Wno-unknown-warning-option

	-fdata-sections
	-ffunction-sections
	-fno-omit-frame-pointer
	-fno-strict-aliasing
	-funwind-tables
	-gfull
)

string(REPLACE ";" " " CLANG_COMMON_FLAGS "${CLANG_COMMON_FLAGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_COMMON_FLAGS} -ffast-math -fno-rtti -fno-exceptions -std=c++11" CACHE STRING "C++ Common Flags" FORCE)
 # Set MSVC option for Visual Studio properties to be displayed correctly
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -D_DEBUG" CACHE STRING "C++ Debug Flags" FORCE)
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -D_RELEASE" CACHE STRING "C++ Release Flags" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Libraries linked by defalut with all C++ applications." FORCE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_COMMON_FLAGS}" CACHE STRING "C Common Flags" FORCE)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -D_DEBUG" CACHE STRING "C Debug Flags" FORCE)
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -D_RELEASE" CACHE STRING "C Release Flags" FORCE)

set(CMAKE_LINK_FLAGS "{$CMAKE_LINK_FLAGS}") #-Wl,-dead_strip

set(CMAKE_SHARED_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Library Profile Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Executable Profile Flags" FORCE)
