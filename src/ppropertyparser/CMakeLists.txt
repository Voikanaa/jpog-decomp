# PPropertyParser.dll reimplementation (issue #25).
add_library(ppropertyparser STATIC
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
target_link_libraries(ppropertyparser PUBLIC toshi_kernel)
