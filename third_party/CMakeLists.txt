# Vendored dependencies (versions, licenses and hashes in README.md). They build with their own
# settings: the project's warning flags and definitions (WIN32_LEAN_AND_MEAN hides the COM headers
# shaderc needs) are dropped for this directory, and their own warnings are not ours to fix.
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS "")
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS "")
if(MSVC)
  add_compile_options(/W0 /utf-8)
else()
  add_compile_options(-w)
endif()

add_library(doctest INTERFACE)
target_include_directories(doctest SYSTEM INTERFACE doctest)

# The portable build's platform stack. The x86 verification build runs on the original engine
# DLLs and needs none of it.
if(JPOG_X86)
  return()
endif()

# Sanitizer builds (linux-asan) instrument our code only: shaderc runs during the build, and
# reports from upstream code would fail it.
foreach(flags CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS)
  string(REGEX REPLACE "-fsanitize[^ ]*|-fno-sanitize[^ ]*" "" ${flags} "${${flags}}")
endforeach()

# --- SDL3 (zlib): window, input, events -------------------------------------------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(sdl3 EXCLUDE_FROM_ALL SYSTEM)

# --- bgfx, bx, bimg (BSD-2) and the shaderc tool, via bgfx.cmake -------------------------------
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_VIDEO OFF CACHE BOOL "" FORCE) # videos go through FFmpeg
# shaderc copies a prebuilt DirectX Shader Compiler from bgfx's tree next to itself after linking.
# We vendor no binaries (JPOG_DXC below fetches it instead), so that one step is dropped while bgfx
# is configured; shaderc loads the compiler only on demand. Every other call passes through.
function(add_custom_command)
  if(JPOG_SKIP_DXCOMPILER_COPY AND ARGV MATCHES "dxcompiler" AND NOT EXISTS "${DXCOMPILER_RUNTIME}")
    return()
  endif()
  _add_custom_command(${ARGV})
endfunction()
set(JPOG_SKIP_DXCOMPILER_COPY ON)
add_subdirectory(bgfx EXCLUDE_FROM_ALL SYSTEM)
set(JPOG_SKIP_DXCOMPILER_COPY OFF)
# --- DirectX Shader Compiler (build tool, Windows only): DXIL shaders for Direct3D 12 ------------
# Not vendored or distributed: Microsoft's license allows using dxil.dll on Windows but not
# sharing it, so each Windows build downloads the pinned release (checked by SHA-256) from
# Microsoft's GitHub into .cache/. With JPOG_DXC=OFF (e.g. offline), Direct3D 12 uses DXBC.
if(WIN32)
  option(JPOG_DXC "Compile DXIL shaders for Direct3D 12 (downloads the DirectX Shader Compiler)" ON)
else()
  set(JPOG_DXC OFF)
endif()
if(JPOG_DXC)
  set(dxc_version 1.9.2609)
  set(dxc_dir "${PROJECT_SOURCE_DIR}/.cache/dxc/${dxc_version}")
  if(NOT EXISTS "${dxc_dir}/bin/x64/dxil.dll")
    message(STATUS "Downloading the DirectX Shader Compiler ${dxc_version}")
    file(DOWNLOAD
      "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v${dxc_version}/dxc_2026_09_29.zip"
      "${dxc_dir}.zip"
      EXPECTED_HASH SHA256=ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1
      TLS_VERIFY ON
      STATUS status)
    list(GET status 0 code)
    if(code)
      file(REMOVE "${dxc_dir}.zip")
      message(FATAL_ERROR "DirectX Shader Compiler download failed (${status}); configure with -DJPOG_DXC=OFF to build without DXIL")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${dxc_dir}.zip" DESTINATION "${dxc_dir}"
      PATTERNS bin/x64/dxcompiler.dll bin/x64/dxil.dll LICENSE-MS.txt LICENSE-LLVM.txt LICENCE-MIT.txt)
    file(REMOVE "${dxc_dir}.zip")
  endif()
  # shaderc loads dxcompiler.dll, which loads dxil.dll to sign the shaders, from its own directory.
  set(shaderc_dir "${CMAKE_BINARY_DIR}/tools")
  set_target_properties(shaderc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${shaderc_dir}")
  file(COPY "${dxc_dir}/bin/x64/dxcompiler.dll" "${dxc_dir}/bin/x64/dxil.dll" DESTINATION "${shaderc_dir}")
endif()

# bgfx_compile_shaders() (only loaded by bgfx.cmake's examples) and the bgfx_shader.sh includes.
include(bgfx/cmake/bgfxToolUtils.cmake)
set(JPOG_BGFX_SHADER_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/bgfx/bgfx/src" CACHE INTERNAL "")

find_package(Threads REQUIRED) # miniaudio and Tracy on Linux/macOS

# --- Dear ImGui (MIT): development menu -------------------------------------------------------
add_library(imgui STATIC EXCLUDE_FROM_ALL
  imgui/imgui.cpp
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp
  imgui/backends/imgui_impl_sdl3.cpp
)
target_include_directories(imgui SYSTEM PUBLIC imgui imgui/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# --- miniaudio (MIT-0 / public domain): audio output ------------------------------------------
add_library(miniaudio STATIC EXCLUDE_FROM_ALL miniaudio/miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC miniaudio)
if(APPLE)
  # Link the audio frameworks instead of loading them at run time.
  target_compile_definitions(miniaudio PUBLIC MA_NO_RUNTIME_LINKING)
  target_link_libraries(miniaudio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
elseif(UNIX)
  target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

# --- FFmpeg 9.0 headers (LGPL-2.1): Bink video, loaded at run time ---------------------------
# Headers only: jpog loads the shared libraries itself (src/shell/FFmpeg.cpp), so nothing links.
add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg SYSTEM INTERFACE ffmpeg/include)

# --- Tracy (BSD-3): profiler client, compiled in only with JPOG_TRACY ---------------------------
option(JPOG_TRACY "Build with the Tracy profiler client" OFF)
if(JPOG_TRACY)
  add_library(tracy STATIC EXCLUDE_FROM_ALL tracy/public/TracyClient.cpp)
  target_compile_definitions(tracy PUBLIC TRACY_ENABLE)
  if(UNIX)
    target_link_libraries(tracy PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
  elseif(WIN32)
    target_link_libraries(tracy PUBLIC ws2_32 dbghelp)
  endif()
else()
  add_library(tracy INTERFACE)
endif()
target_include_directories(tracy SYSTEM INTERFACE tracy/public)

# --- stb_image_write (public domain / MIT): screenshots and texture dumps ----------------------
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb)
