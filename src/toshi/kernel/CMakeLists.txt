# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  TFreeList.cpp
  TFreeList.h
  TMath.cpp
  TMath.h
  TMatrix44.cpp
  TMatrix44.h
  TMemory.cpp
  TMemory.h
  TPlane.cpp
  TPlane.h
  TQuaternion.cpp
  TQuaternion.h
  TVector2.cpp
  TVector2.h
  TVector3.cpp
  TVector3.h
  TVector4.cpp
  TVector4.h
)
target_link_libraries(toshi_kernel PUBLIC jpog_common)
