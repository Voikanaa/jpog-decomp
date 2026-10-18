cmake_minimum_required(VERSION 3.28)
project(JPOG LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(JPOG_X86 ON)
else()
  set(JPOG_X86 OFF)
endif()

# x86 builds produce jpog.exe linked against the original engine DLLs (hybrid), plus the
# differential tests. JPOG_HOOK additionally builds the (deferred) hook DLL for the original exe.
option(JPOG_HOOK "Also build the x86 hook DLL for the original game" OFF)
if(JPOG_HOOK AND NOT JPOG_X86)
  message(FATAL_ERROR "JPOG_HOOK requires an x86 (32-bit) build")
endif()

option(JPOG_BUILD_TESTS "Build the unit and differential tests" ON)
# x86: install our TKernelInterface.dll into the game copy instead of the original (issue #24);
# OFF restores the original there.
option(JPOG_REPLACE_KERNEL "Run the game copy on our TKernelInterface.dll" ON)

# Location of the original game install. Differential tests load the original DLLs from here
# (overridable at run time with the JPOG_ORIGINAL_DIR environment variable); they are skipped
# when it is missing, e.g. in CI.
set(JPOG_ORIGINAL_DIR "${CMAKE_SOURCE_DIR}/JPOG" CACHE PATH "Original game install directory")
# Local working copy of the game; jpog.exe is copied here after each build (x86).
set(JPOG_GAME_DIR "${CMAKE_SOURCE_DIR}/game" CACHE PATH "Local game working directory")

add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

add_library(jpog_common INTERFACE)
target_include_directories(jpog_common INTERFACE src)

add_subdirectory(third_party)
add_subdirectory(src/toshi/kernel)

if(JPOG_X86)
  include(cmake/OriginalModules.cmake)
  jpog_original_module(TKernelInterface)
  jpog_original_module(TApplication)
  jpog_original_module(TRenderD3DInterface)
  jpog_original_module(TGuiInterface)
  jpog_original_module(PPropertyParser)
  add_subdirectory(src/game)
endif()

if(JPOG_HOOK)
  add_subdirectory(src/hook)
endif()

if(JPOG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
