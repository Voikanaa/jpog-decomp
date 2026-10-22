cmake_minimum_required(VERSION 3.28)
project(JPOG LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# No C++20 modules: skip the dependency scan (its .modmap files also confuse clang-tidy).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# The x86 build is the Windows verification build: it runs on and tests against the original
# 32-bit DLLs. Every other build is a portable one (Windows x64, Linux, macOS).
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "32-bit builds are only supported on Windows with MSVC")
  endif()
  set(JPOG_X86 ON)
else()
  set(JPOG_X86 OFF)
endif()

# x86 builds produce jpog.exe linked against the original engine DLLs (hybrid), plus the
# differential tests. The hook DLL for the original exe is deferred (#2).

option(JPOG_BUILD_TESTS "Build the unit and differential tests" ON)
# x86: install our TKernelInterface.dll into the game copy instead of the original (issue #24);
# OFF restores the original there.
option(JPOG_REPLACE_KERNEL "Run the game copy on our TKernelInterface.dll" ON)
# Likewise for our PPropertyParser.dll (issue #25).
option(JPOG_REPLACE_PPROPERTYPARSER "Run the game copy on our PPropertyParser.dll" ON)

# Location of the original game install. Differential tests load the original DLLs from here
# (overridable at run time with the JPOG_ORIGINAL_DIR environment variable); they are skipped
# when it is missing, e.g. in CI.
set(JPOG_ORIGINAL_DIR "${CMAKE_SOURCE_DIR}/JPOG" CACHE PATH "Original game install directory")
# Local working copy of the game; jpog.exe is copied here after each build (x86).
set(JPOG_GAME_DIR "${CMAKE_SOURCE_DIR}/game" CACHE PATH "Local game working directory")

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
  # No FMA contraction or fast-math: float results must match the original's (docs/plan.md).
  add_compile_options(-Wall -Wextra -ffp-contract=off -fno-fast-math)
  # Fields kept only for the original layout are often unused.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-unused-private-field)
  endif()
endif()
if(WIN32)
  add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_library(jpog_common INTERFACE)
target_include_directories(jpog_common INTERFACE src)

add_subdirectory(third_party)

# Treat warnings as errors in our own code (CI turns this on); vendored code above is exempt.
option(JPOG_WERROR "Treat compiler warnings as errors" OFF)
if(JPOG_WERROR)
  if(MSVC)
    add_compile_options(/WX)
  else()
    add_compile_options(-Werror)
  endif()
endif()

add_subdirectory(src/platform)
add_subdirectory(src/toshi/kernel)
add_subdirectory(src/ppropertyparser)

if(JPOG_X86)
  include(cmake/OriginalModules.cmake)
  jpog_original_module(TKernelInterface)
  jpog_original_module(TApplication)
  jpog_original_module(TRenderInterface)
  jpog_original_module(TRenderD3DInterface)
  jpog_original_module(TGuiInterface)
  jpog_original_module(PPropertyParser)
  add_subdirectory(src/game)
endif()

if(JPOG_BUILD_TESTS)
  enable_testing()
endif()

if(NOT JPOG_X86)
  add_subdirectory(src/shell)
endif()

if(JPOG_BUILD_TESTS)
  add_subdirectory(tests)
endif()
