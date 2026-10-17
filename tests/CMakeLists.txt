add_executable(jpog_tests
  main.cpp
  support/Differential.h
  support/FloatCompare.h
  support/OriginalModule.cpp
  support/OriginalModule.h
  kernel/TMathTests.cpp
  kernel/TMatrix44Tests.cpp
  kernel/TMemoryTests.cpp
  kernel/TQuaternionTests.cpp
  kernel/TVectorTests.cpp
)
target_include_directories(jpog_tests PRIVATE .)
target_link_libraries(jpog_tests PRIVATE toshi_kernel doctest)
target_compile_definitions(jpog_tests PRIVATE JPOG_ORIGINAL_DIR="${JPOG_ORIGINAL_DIR}")

add_test(NAME jpog_tests COMMAND jpog_tests)
