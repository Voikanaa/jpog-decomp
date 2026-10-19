# Import libraries for the original (unprotected) engine DLLs, generated from the committed
# export tables (symbols/<Module>.exports.csv) by tools/make_import_lib.py. x86 only: the
# originals are 32-bit. Our executable links against these for everything not yet reimplemented.
#
#   jpog_original_module(TKernelInterface)  ->  target original::TKernelInterface

find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(jpog_original_module module)
  set(lib "${CMAKE_BINARY_DIR}/original/${module}.lib")
  add_custom_command(
    OUTPUT "${lib}"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/make_import_lib.py" ${module} "${lib}"
    DEPENDS "${CMAKE_SOURCE_DIR}/symbols/${module}.exports.csv" "${CMAKE_SOURCE_DIR}/tools/make_import_lib.py"
            "${CMAKE_SOURCE_DIR}/tools/coverage.py"
    COMMENT "Generating import library for original ${module}.dll"
    VERBATIM)
  add_custom_target(original_${module}_lib DEPENDS "${lib}")

  add_library(original_${module} INTERFACE)
  target_link_libraries(original_${module} INTERFACE "${lib}")
  add_dependencies(original_${module} original_${module}_lib)
  add_library(original::${module} ALIAS original_${module})
endfunction()
