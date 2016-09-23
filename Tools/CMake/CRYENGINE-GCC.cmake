if(CMAKE_BUILD_TYPE MATCHES DEBUG)
	add_definitions(-D_DEBUG=1)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

set(GCC_COMMON_FLAGS 
-Wall  
-Werror
-ffast-math 
-Wno-char-subscripts 
-Wno-unknown-pragmas 
-Wno-unused-variable 
-Wno-unused-value 
-Wno-parentheses 
-Wno-switch 
-Wno-unused-function 
-Wno-multichar 
-Wno-format-security 
-Wno-empty-body 
-Wno-comment 
-Wno-char-subscripts 
-Wno-sign-compare 
-Wno-narrowing 
-Wno-write-strings 
-Wno-format 
-Wno-strict-aliasing 
-Wno-unused-but-set-variable 
-Wno-maybe-uninitialized 
-Wno-strict-overflow 
-Wno-uninitialized 
-Wno-unused-local-typedefs 
-Wno-deprecated 
-Wno-unused-result 
-Wno-sizeof-pointer-memaccess 
-Wno-array-bounds
-gfull

-ffunction-sections
-fdata-sections
-funwind-tables
-fno-omit-frame-pointer
-fno-strict-aliasing
-funswitch-loops

-D_HAS_C9X
)

string(REPLACE ";" " " GCC_COMMON_FLAGS "${GCC_COMMON_FLAGS}")

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${GCC_COMMON_FLAGS}
-fno-rtti 
-fno-exceptions 
-Wno-invalid-offsetof 
-Wno-reorder 
-Wno-conversion-null 
-Wno-overloaded-virtual 
-Wno-c++0x-compat 
-std=c++11 
-std=gnu++11

CACHE STRING "C++ Flags" FORCE
 )
 string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
 
 # Set MSVC option for Visual Studio properties to be displayed correctly
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -D_DEBUG" CACHE STRING "C++ Debug Flags" FORCE)
set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -D_RELEASE" CACHE STRING "C++ Release Flags" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "Libraries linked by defalut with all C++ applications." FORCE)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COMMON_FLAGS}" )
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /GS /Od /Ob0 /RTC1 -D_DEBUG" CACHE STRING "C Debug Flags" FORCE)
set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -O2 -D_PROFILE" CACHE STRING "C++ Profile Flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GS- /Ox /Ob2 /Oi /Ot -D_RELEASE" CACHE STRING "C Release Flags" FORCE)

set(CMAKE_LINK_FLAGS "{$CMAKE_LINK_FLAGS} -Wl, --gc-sections")


set(CMAKE_SHARED_LINKER_FLAGS_PROFILE ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Library Profile Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "Linker Executable Profile Flags" FORCE)