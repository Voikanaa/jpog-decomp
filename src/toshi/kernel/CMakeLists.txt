# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  StringPoolArray.h
  TCString.cpp
  TCString.h
  TCStringPool.cpp
  TCStringPool.h
  TFreeList.cpp
  TFreeList.h
  TGenericDList.cpp
  TGenericDList.h
  TGenericNodeList.cpp
  TGenericNodeList.h
  TMath.cpp
  TMath.h
  TMatrix44.cpp
  TMatrix44.h
  TMemory.cpp
  TMemory.h
  TObject.cpp
  TObject.h
  TPlane.cpp
  TPlane.h
  TQuaternion.cpp
  TQuaternion.h
  TSystem.cpp
  TSystem.h
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
