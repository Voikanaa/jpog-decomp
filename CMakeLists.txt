cmake_minimum_required(VERSION 3.28)
project(JPOG LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(JPOG_X86 ON)
else()
  set(JPOG_X86 OFF)
endif()

# JPOG_HOOK: build the reimplementation as a DLL injected into the original 32-bit game.
option(JPOG_HOOK "Build the x86 hook DLL instead of the standalone game" ${JPOG_X86})
if(JPOG_HOOK AND NOT JPOG_X86)
  message(FATAL_ERROR "JPOG_HOOK requires an x86 (32-bit) build")
endif()

option(JPOG_BUILD_TESTS "Build the unit and differential tests" ON)

# Location of the original game install. Differential tests load the original DLLs from here
# (overridable at run time with the JPOG_ORIGINAL_DIR environment variable); they are skipped
# when it is missing, e.g. in CI.
set(JPOG_ORIGINAL_DIR "${CMAKE_SOURCE_DIR}/JPOG" CACHE PATH "Original game install directory")

add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX $<$<BOOL:${JPOG_HOOK}>:JPOG_HOOK=1>)

add_library(jpog_common INTERFACE)
target_include_directories(jpog_common INTERFACE src)

add_subdirectory(third_party)
add_subdirectory(src/toshi/kernel)

if(JPOG_HOOK)
  add_subdirectory(src/hook)
endif()

if(JPOG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
