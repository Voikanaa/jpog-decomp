# x86-only DLL loaded into the original SimJP.exe; installs hooks that redirect
# original functions to our reimplementations.
add_library(jpog_hook SHARED
  DllMain.cpp
)
target_link_libraries(jpog_hook PRIVATE jpog_common)
