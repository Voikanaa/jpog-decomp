# The portable game shell `jpog` (Windows x64, Linux, macOS). The x86 build has its own jpog.exe
# (src/game) on the original engine DLLs.

# Shaders, compiled by bgfx's shaderc into headers. Profiles per platform: Direct3D 12 (DXIL,
# Shader Model 6, with JPOG_DXC), Direct3D 11 (DXBC), Vulkan (SPIR-V), OpenGL (GLSL 4.30: bgfx
# needs OpenGL 4.3) and Metal.
if(WIN32)
  set(profiles s_5_0 spirv 430)
  if(JPOG_DXC)
    list(APPEND profiles s_6_0)
  endif()
elseif(APPLE)
  set(profiles metal)
else()
  set(profiles spirv 430)
endif()
set(shader_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
foreach(type VERTEX FRAGMENT)
  string(TOLOWER ${type} kind)
  string(SUBSTRING ${kind} 0 1 prefix)
  bgfx_compile_shaders(
    TYPE ${type}
    SHADERS shaders/${prefix}s_imgui.sc
    VARYING_DEF "${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc"
    OUTPUT_DIR "${shader_dir}"
    OUT_FILES_VAR ${kind}_shaders
    INCLUDE_DIRS "${JPOG_BGFX_SHADER_INCLUDE}"
    PROFILES ${profiles}
    AS_HEADERS
  )
endforeach()

add_executable(jpog
  main.cpp
  Audio.cpp
  Audio.h
  BgfxCallback.cpp
  BgfxCallback.h
  FFmpeg.cpp
  FFmpeg.h
  ImGuiRenderer.cpp
  ImGuiRenderer.h
  Shaders.cpp
  Shaders.h
  Shell.cpp
  Shell.h
  ${vertex_shaders}
  ${fragment_shaders}
)
target_include_directories(jpog PRIVATE "${PROJECT_SOURCE_DIR}/src" "${shader_dir}")
if(JPOG_DXC)
  target_compile_definitions(jpog PRIVATE JPOG_SHADERS_DXIL)
endif()
target_link_libraries(jpog PRIVATE toshi_kernel jpog_platform SDL3::SDL3-static bgfx bx imgui miniaudio ffmpeg stb tracy)

if(JPOG_BUILD_TESTS)
  # Headless start-up and 60 frames on every OS (CI has no display or audio device).
  add_test(NAME jpog_smoke COMMAND jpog --smoke 60)
  set_tests_properties(jpog_smoke PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/tests/lsan.supp"
    PASS_REGULAR_EXPRESSION "smoke run finished after 60 frames")
endif()
