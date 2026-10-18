# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  SortedArray.h
  StringPoolArray.h
  TArray.h
  TAABox.cpp
  TAABox.h
  TBitArray.cpp
  TBitArray.h
  TCString.cpp
  TCString.h
  TCStringPool.cpp
  TCStringPool.h
  TDebug.cpp
  TDebug.h
  TDLL.cpp
  TDLL.h
  TEvent.cpp
  TEvent.h
  TFile.cpp
  TFile.h
  TFileLexer.cpp
  TFileLexer.h
  TFreeList.cpp
  TFreeList.h
  TGenericDList.cpp
  TGenericDList.h
  TGenericFifo.cpp
  TGenericFifo.h
  TGenericNodeList.cpp
  TGenericNodeList.h
  TGenericNTree.cpp
  TGenericNTree.h
  TGenericPool.cpp
  TGenericPool.h
  TGenericPriList.cpp
  TGenericPriList.h
  THPTimer.cpp
  THPTimer.h
  TISOFile.cpp
  TISOFile.h
  TKernelInterface.cpp
  TKernelInterface.h
  TLocale.cpp
  TLocale.h
  TMap.h
  TMath.cpp
  TMath.h
  TMathGeometry.cpp
  TMinterm.cpp
  TMinterm.h
  TMatrix44.cpp
  TMatrix44.h
  TMemFile.cpp
  TMemFile.h
  TMemory.cpp
  TMemory.h
  TMutex.cpp
  TMutex.h
  TNativeFile.cpp
  TNativeFile.h
  TNodeTree.h
  TNullFile.cpp
  TNullFile.h
  TObject.cpp
  TObject.h
  TPile.cpp
  TPile.h
  TPlane.cpp
  TPlane.h
  TProfiler.cpp
  TProfiler.h
  TQuaternion.cpp
  TQuaternion.h
  TRandom.cpp
  TRandom.h
  TRefCounted.cpp
  TRefCounted.h
  TScheduler.cpp
  TScheduler.h
  TSphere.cpp
  TSphere.h
  TSystem.cpp
  TSystem.h
  TThread.cpp
  TThread.h
  TTimer.cpp
  TTimer.h
  TUser.cpp
  TUser.h
  TVector2.cpp
  TVector2.h
  TVector3.cpp
  TVector3.h
  TVector4.cpp
  TVector4.h
  TWString.cpp
  TWString.h
  TWStringPool.cpp
  TWStringPool.h
)
target_link_libraries(toshi_kernel PUBLIC jpog_common)
# The original relies on strncpy/wcsncpy semantics (zero padding, no forced terminator).
target_compile_definitions(toshi_kernel PRIVATE _CRT_SECURE_NO_WARNINGS)

# x86: a drop-in replacement for the original TKernelInterface.dll, exporting the original names
# and ordinals (issue #24). With JPOG_REPLACE_KERNEL (default) it replaces the original in the
# game copy.
if(JPOG_X86)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(def "${CMAKE_CURRENT_BINARY_DIR}/TKernelInterface.def")
  add_custom_command(
    OUTPUT "${def}"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/make_module_def.py" TKernelInterface "${def}"
    DEPENDS "${CMAKE_SOURCE_DIR}/symbols/TKernelInterface.exports.csv" "${CMAKE_SOURCE_DIR}/tools/make_module_def.py"
            "${CMAKE_SOURCE_DIR}/tools/coverage.py"
    COMMENT "Generating TKernelInterface.def"
    VERBATIM)
  add_library(toshi_kernel_dll SHARED dll/DllMain.cpp "${def}")
  target_link_libraries(toshi_kernel_dll PRIVATE toshi_kernel)
  # The import library gets its own name: jpog.exe links the original's (cmake/OriginalModules.cmake).
  set_target_properties(toshi_kernel_dll PROPERTIES OUTPUT_NAME TKernelInterface ARCHIVE_OUTPUT_NAME toshi_kernel_dll)

  if(EXISTS "${JPOG_GAME_DIR}/TKernelInterface.dll")
    if(JPOG_REPLACE_KERNEL)
      add_custom_command(TARGET toshi_kernel_dll POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:toshi_kernel_dll>" "${JPOG_GAME_DIR}/TKernelInterface.dll"
        COMMENT "Installing our TKernelInterface.dll into ${JPOG_GAME_DIR}")
    elseif(EXISTS "${JPOG_ORIGINAL_DIR}/TKernelInterface.dll")
      add_custom_command(TARGET toshi_kernel_dll POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${JPOG_ORIGINAL_DIR}/TKernelInterface.dll" "${JPOG_GAME_DIR}/TKernelInterface.dll"
        COMMENT "Restoring the original TKernelInterface.dll in ${JPOG_GAME_DIR}")
    endif()
  endif()
endif()
