# OS layer under the kernel: Win32 on Windows, POSIX elsewhere. No third-party dependencies, so
# the x86 drop-in DLLs link it statically without new imports.
add_library(jpog_platform STATIC
  CRuntime.cpp
  CRuntime.h
  System.h
  Thread.h
)
if(WIN32)
  target_sources(jpog_platform PRIVATE win32/System.cpp win32/Thread.cpp)
else()
  target_sources(jpog_platform PRIVATE posix/System.cpp posix/Thread.cpp)
  find_package(Threads REQUIRED)
  target_link_libraries(jpog_platform PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
target_link_libraries(jpog_platform PUBLIC jpog_common)
