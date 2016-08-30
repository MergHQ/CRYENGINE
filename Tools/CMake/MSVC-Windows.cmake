set(CMAKE_CONFIGURATION_TYPES Debug Profile Release)
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "Reset the configurations to what we need" FORCE)

if (CMAKE_CL_64)
	#64bit
	set( BUILD_CPU_ARCHITECTURE x64 )
	set( BUILD_PLATFORM Win64 )
	set(WIN64 1)
	set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/win_x64")
else()
	#32it
	set( BUILD_CPU_ARCHITECTURE x86 )
	set( BUILD_PLATFORM Win32 )
	set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/win_x86")
endif()

MESSAGE(STATUS "BUILD_CPU_ARCHITECTURE = ${BUILD_CPU_ARCHITECTURE}" )

# -------------------------------------------------------------------
# Full Optimization (/Ox)
# Enable Intrinsic Functions (/Oi)
# Favor Fast Code  (/Ot)

# Streaming SIMD Extensions (/arch:SSE2)
# Set Float Point Precision Model: Fast (/fp:fast)
# Enable Multi-processor Compilation (/MP)
# Enable String Pooling (/GF)
# Enable Function-level Linking (/Gy)
# Treat Warnings as Errors (/Wx)

# Suppress warning C4996 (/DNO_WARN_MBCS_MFC_DEPRECATION)

# Suppress warning C4251 (/wd4251)
# Suppress warning C4275 (/wd4275)
# Suppress warning C4257 (/wd4251)

if (BUILD_PLATFORM STREQUAL Win32)
	set(CMAKE_CXX_FLAGS "/W3 /fp:fast /MP /GF /Gy /DWIN32 /D_WINDOWS /W3 /GR- /Gd /Zm250 /Zc:forScope /arch:SSE2 /DNO_WARN_MBCS_MFC_DEPRECATION /wd4251 /wd4275 /Gd /utf-8 /Wv:18" CACHE STRING "C++ Flags" FORCE)
else()
	set(CMAKE_CXX_FLAGS "/W3 /fp:fast /MP /GF /Gy /DWIN32 /D_WINDOWS /W3 /GR- /Gd /Zm250 /Zc:forScope /arch:AVX /DNO_WARN_MBCS_MFC_DEPRECATION /wd4251 /wd4275 /Gd /utf-8 /Wv:18" CACHE STRING "C++ Flags" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "/Zi /MDd /Od /Ob0 /Oy- /RTC1 /GS /bigobj /D_DEBUG" CACHE STRING "C++ Flags" FORCE)

# Profile flags
set(CMAKE_CXX_FLAGS_PROFILE "/Zi /MD /Ox /Oi /Ot /Ob2 /Oy- /GS- /D NDEBUG /D_PROFILE" CACHE STRING "C++ Flags" FORCE)
set(CMAKE_C_FLAGS_PROFILE "/Zi /MD /Ox /Oi /Ot /Ob2 /Oy- /GS- /D NDEBUG /D_PROFILE" CACHE STRING "C Flags" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "/debug /INCREMENTAL" CACHE STRING "C++ Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "/debug /INCREMENTAL" CACHE STRING "C++ Flags" FORCE)

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "/MD /Ox /Oi /Ot /Ob2 /Oy- /GS- /D NDEBUG /D_RELEASE /D PURE_CLIENT" CACHE STRING "C++ Flags" FORCE)
#set(CMAKE_SHARED_LINKER_FLAGS_RELEASE ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} CACHE STRING "C++ Flags" FORCE)
#set(CMAKE_EXE_LINKER_FLAGS_RELEASE ${CMAKE_EXE_LINKER_FLAGS_RELEASE} CACHE STRING "C++ Flags" FORCE)
# -------------------------------------------------------------------
