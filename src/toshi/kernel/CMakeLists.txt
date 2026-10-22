# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  TDebug.cpp
  TDebug.h
  TDLL.cpp
  TDLL.h
  TEvent.cpp
  TEvent.h
  TKernelInterface.cpp
  TKernelInterface.h
  TLocale.cpp
  TLocale.h
  TMinterm.cpp
  TMinterm.h
  TObject.cpp
  TObject.h
  TProfiler.cpp
  TProfiler.h
  TRefCounted.cpp
  TRefCounted.h
  TScheduler.cpp
  TScheduler.h
  TSystem.cpp
  TSystem.h
  TUser.cpp
  TUser.h
  containers/SortedArray.h
  containers/TArray.h
  containers/TBitArray.cpp
  containers/TBitArray.h
  containers/TGenericDList.cpp
  containers/TGenericDList.h
  containers/TGenericFifo.cpp
  containers/TGenericFifo.h
  containers/TGenericNodeList.cpp
  containers/TGenericNodeList.h
  containers/TGenericNTree.cpp
  containers/TGenericNTree.h
  containers/TGenericPool.cpp
  containers/TGenericPool.h
  containers/TGenericPriList.cpp
  containers/TGenericPriList.h
  containers/TMap.h
  containers/TNodeTree.h
  file/TFile.cpp
  file/TFile.h
  file/TFileLexer.cpp
  file/TFileLexer.h
  file/TISOFile.cpp
  file/TISOFile.h
  file/TMemFile.cpp
  file/TMemFile.h
  file/TNativeFile.cpp
  file/TNativeFile.h
  file/TNullFile.cpp
  file/TNullFile.h
  math/TAABox.cpp
  math/TAABox.h
  math/TMath.cpp
  math/TMath.h
  math/TMathGeometry.cpp
  math/TMatrix44.cpp
  math/TMatrix44.h
  math/TPlane.cpp
  math/TPlane.h
  math/TQuaternion.cpp
  math/TQuaternion.h
  math/TRandom.cpp
  math/TRandom.h
  math/TSphere.cpp
  math/TSphere.h
  math/TVector2.cpp
  math/TVector2.h
  math/TVector3.cpp
  math/TVector3.h
  math/TVector4.cpp
  math/TVector4.h
  memory/TFreeList.cpp
  memory/TFreeList.h
  memory/TMemory.cpp
  memory/TMemory.h
  memory/TPile.cpp
  memory/TPile.h
  strings/StringPoolArray.h
  strings/TCString.cpp
  strings/TCString.h
  strings/TCStringPool.cpp
  strings/TCStringPool.h
  strings/TWString.cpp
  strings/TWString.h
  strings/TWStringPool.cpp
  strings/TWStringPool.h
  thread/TMutex.cpp
  thread/TMutex.h
  thread/TThread.cpp
  thread/TThread.h
  time/THPTimer.cpp
  time/THPTimer.h
  time/TTimer.cpp
  time/TTimer.h
)
target_link_libraries(toshi_kernel PUBLIC jpog_common jpog_platform)
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
