# TKernelInterface.dll reimplementation.
add_library(toshi_kernel STATIC
  TMath.cpp
  TMath.h
)
target_link_libraries(toshi_kernel PUBLIC jpog_common)
