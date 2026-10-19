# PPropertyParser.dll reimplementation (issue #25).
set(ppropertyparser_sources
  PProperties.cpp
  PProperties.h
  PPropertyBlock.cpp
  PPropertyBlock.h
  PPropertyName.cpp
  PPropertyName.h
  PPropertyReader.cpp
  PPropertyReader.h
  PPropertyValue.cpp
  PPropertyValue.h
  PPropertyWriter.cpp
  PPropertyWriter.h
)
add_library(ppropertyparser STATIC ${ppropertyparser_sources})
target_link_libraries(ppropertyparser PUBLIC toshi_kernel)

# x86: a drop-in replacement for the original PPropertyParser.dll with the original export names
# and ordinals. It imports the kernel from TKernelInterface.dll (ours or the original, which
# export the same names), so the kernel's data is declared dllimport (JPOG_IMPORT_ENGINE_DATA).
# With JPOG_REPLACE_PPROPERTYPARSER (default) it replaces the original in the game copy.
if(JPOG_X86)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(def "${CMAKE_CURRENT_BINARY_DIR}/PPropertyParser.def")
  add_custom_command(
    OUTPUT "${def}"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/make_module_def.py" PPropertyParser "${def}"
    DEPENDS "${CMAKE_SOURCE_DIR}/symbols/PPropertyParser.exports.csv" "${CMAKE_SOURCE_DIR}/tools/make_module_def.py"
            "${CMAKE_SOURCE_DIR}/tools/coverage.py"
    COMMENT "Generating PPropertyParser.def"
    VERBATIM)
  # Global new/delete go to the kernel heap, as in jpog.exe.
  add_library(ppropertyparser_dll SHARED ${ppropertyparser_sources} dll/DllMain.cpp
                                         "${CMAKE_SOURCE_DIR}/src/game/Memory.cpp" "${def}")
  target_link_libraries(ppropertyparser_dll PRIVATE jpog_common toshi_kernel_dll)
  target_compile_definitions(ppropertyparser_dll PRIVATE JPOG_IMPORT_ENGINE_DATA)
  # The import library gets its own name: jpog.exe links the original's (cmake/OriginalModules.cmake).
  set_target_properties(ppropertyparser_dll PROPERTIES OUTPUT_NAME PPropertyParser
                                                       ARCHIVE_OUTPUT_NAME ppropertyparser_dll)

  if(EXISTS "${JPOG_GAME_DIR}/PPropertyParser.dll")
    if(JPOG_REPLACE_PPROPERTYPARSER)
      add_custom_command(TARGET ppropertyparser_dll POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ppropertyparser_dll>" "${JPOG_GAME_DIR}/PPropertyParser.dll"
        COMMENT "Installing our PPropertyParser.dll into ${JPOG_GAME_DIR}")
    elseif(EXISTS "${JPOG_ORIGINAL_DIR}/PPropertyParser.dll")
      add_custom_command(TARGET ppropertyparser_dll POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${JPOG_ORIGINAL_DIR}/PPropertyParser.dll" "${JPOG_GAME_DIR}/PPropertyParser.dll"
        COMMENT "Restoring the original PPropertyParser.dll in ${JPOG_GAME_DIR}")
    endif()
  endif()
endif()
