add_executable(jpog_tests
  main.cpp
  support/Differential.h
  support/FloatCompare.h
  support/OriginalModule.cpp
  support/OriginalModule.h
  game/AOptionsTests.cpp
  ../src/game/AOptions.cpp
  kernel/TDebugTests.cpp
  kernel/TEventTests.cpp
  kernel/TLocaleTests.cpp
  kernel/TMintermTests.cpp
  kernel/TObjectTests.cpp
  kernel/TSchedulerTests.cpp
  kernel/containers/TContainerTests.cpp
  kernel/containers/TPoolTests.cpp
  kernel/file/TFileLexerTests.cpp
  kernel/file/TFileTests.cpp
  kernel/file/TISOFileTests.cpp
  kernel/math/TBoundsTests.cpp
  kernel/math/TMathGeometryTests.cpp
  kernel/math/TMathTests.cpp
  kernel/math/TMatrix44Tests.cpp
  kernel/math/TQuaternionTests.cpp
  kernel/math/TRandomTests.cpp
  kernel/math/TVectorTests.cpp
  kernel/memory/TMemoryTests.cpp
  kernel/strings/TCStringPoolTests.cpp
  kernel/strings/TStringTests.cpp
  kernel/time/TTimerTests.cpp
  kernel/time/TTimeTests.cpp
  platform/PlatformTests.cpp
  ppropertyparser/PPropertyNameTests.cpp
  ppropertyparser/PPropertyReaderTests.cpp
  ppropertyparser/PPropertyValueTests.cpp
  ppropertyparser/PPropertyWriterTests.cpp
)
target_include_directories(jpog_tests PRIVATE .)
target_link_libraries(jpog_tests PRIVATE toshi_kernel ppropertyparser doctest)
target_compile_definitions(jpog_tests PRIVATE JPOG_ORIGINAL_DIR="${JPOG_ORIGINAL_DIR}")

add_test(NAME jpog_tests COMMAND jpog_tests)
