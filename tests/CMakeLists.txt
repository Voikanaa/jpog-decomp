add_executable(jpog_tests
  main.cpp
  support/Differential.h
  support/FloatCompare.h
  support/OriginalModule.cpp
  support/OriginalModule.h
  game/AOptionsTests.cpp
  ../src/game/AOptions.cpp
  kernel/TBoundsTests.cpp
  kernel/TCStringPoolTests.cpp
  kernel/TContainerTests.cpp
  kernel/TDebugTests.cpp
  kernel/TEventTests.cpp
  kernel/TFileLexerTests.cpp
  kernel/TFileTests.cpp
  kernel/TISOFileTests.cpp
  kernel/TLocaleTests.cpp
  kernel/TMathGeometryTests.cpp
  kernel/TMathTests.cpp
  kernel/TMatrix44Tests.cpp
  kernel/TMemoryTests.cpp
  kernel/TMintermTests.cpp
  kernel/TObjectTests.cpp
  kernel/TPoolTests.cpp
  kernel/TQuaternionTests.cpp
  kernel/TRandomTests.cpp
  kernel/TSchedulerTests.cpp
  kernel/TStringTests.cpp
  kernel/TTimerTests.cpp
  kernel/TTimeTests.cpp
  kernel/TVectorTests.cpp
  ppropertyparser/PPropertyNameTests.cpp
  ppropertyparser/PPropertyReaderTests.cpp
  ppropertyparser/PPropertyValueTests.cpp
  ppropertyparser/PPropertyWriterTests.cpp
)
target_include_directories(jpog_tests PRIVATE .)
target_link_libraries(jpog_tests PRIVATE toshi_kernel ppropertyparser doctest)
target_compile_definitions(jpog_tests PRIVATE JPOG_ORIGINAL_DIR="${JPOG_ORIGINAL_DIR}")

add_test(NAME jpog_tests COMMAND jpog_tests)
