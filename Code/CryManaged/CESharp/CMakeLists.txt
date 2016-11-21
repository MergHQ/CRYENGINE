cmake_minimum_required(VERSION 3.5)
project(CryEngine_CSharp NONE)

option(OPTION_BUILD_CSHARP_WITH_MCS "Build C# with Mono Compiler" ON)

add_subdirectory(Core)
add_subdirectory(Core.Tests)
add_subdirectory(Core.UI)