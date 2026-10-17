# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  TMath.cpp
  TMath.h
  TVector2.cpp
  TVector2.h
  TVector3.cpp
  TVector3.h
  TVector4.cpp
  TVector4.h
)
target_link_libraries(toshi_kernel PUBLIC jpog_common)
