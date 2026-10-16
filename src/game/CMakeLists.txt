# jpog.exe: our reimplementation of SimJP.exe. For now (x86) it links against the original engine
# DLLs for every engine function we have not reimplemented yet.
add_executable(jpog WIN32
  AApplication.cpp
  AApplication.h
  CrashHandler.cpp
  CrashHandler.h
  main.cpp
)
target_link_libraries(jpog PRIVATE jpog_common original::TKernelInterface original::TApplication dbghelp)

# The engine loads its modules, shaders and data relative to the working directory, so the game
# runs from the local game copy (gitignored). Copy the exe there after each build when it exists.
if(EXISTS "${JPOG_GAME_DIR}/TKernelInterface.dll")
  add_custom_command(TARGET jpog POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jpog>" "${JPOG_GAME_DIR}/jpog.exe"
    COMMENT "Copying jpog.exe to ${JPOG_GAME_DIR}")
endif()
