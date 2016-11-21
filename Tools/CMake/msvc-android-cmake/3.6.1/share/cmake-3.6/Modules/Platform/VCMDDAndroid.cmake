include(Platform/Linux)

set(CMAKE_TRY_COMPILE_EXECUTABLE_SUFFIX ".so")
set(CMAKE_TRY_COMPILE_EXECUTABLE_PREFIX "lib")
set(CMAKE_TRY_COMPILE_OUTPUT_TYPE "shared")

# Android has soname, but binary names must end in ".so" so we cannot append
# a version number.  Also we cannot portably represent symlinks on the host.
set(CMAKE_PLATFORM_NO_VERSIONED_SONAME 1)

# Android reportedly ignores RPATH, and we cannot predict the install
# location anyway.
set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "")
