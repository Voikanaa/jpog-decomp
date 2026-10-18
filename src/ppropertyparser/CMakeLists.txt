# PPropertyParser.dll reimplementation (issue #25).
add_library(ppropertyparser STATIC
  PProperties.h
  PPropertyName.cpp
  PPropertyName.h
)
target_link_libraries(ppropertyparser PUBLIC toshi_kernel)
