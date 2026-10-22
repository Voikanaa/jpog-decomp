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
# shaderc copies the prebuilt DirectX Shader Compiler next to itself after linking. We vendor no
# binaries and need no DXIL (D3D12) shaders, so that one step is dropped while bgfx is configured;
# shaderc loads the compiler only on demand. Every other call passes through unchanged.
function(add_custom_command)
  if(JPOG_SKIP_DXCOMPILER_COPY AND ARGV MATCHES "dxcompiler" AND NOT EXISTS "${DXCOMPILER_RUNTIME}")
    return()
  endif()
  _add_custom_command(${ARGV})
endfunction()
set(JPOG_SKIP_DXCOMPILER_COPY ON)
add_subdirectory(bgfx EXCLUDE_FROM_ALL SYSTEM)
set(JPOG_SKIP_DXCOMPILER_COPY OFF)

# --- Dear ImGui (MIT): development menu -------------------------------------------------------
add_library(imgui STATIC
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
add_library(miniaudio STATIC miniaudio/miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC miniaudio)
if(APPLE)
  # Link the audio frameworks instead of loading them at run time.
  target_compile_definitions(miniaudio PUBLIC MA_NO_RUNTIME_LINKING)
  target_link_libraries(miniaudio PUBLIC "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
elseif(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
endif()

# --- Tracy (BSD-3): profiler client, compiled in only with JPOG_TRACY ---------------------------
option(JPOG_TRACY "Build with the Tracy profiler client" OFF)
if(JPOG_TRACY)
  add_library(tracy STATIC tracy/public/TracyClient.cpp)
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
