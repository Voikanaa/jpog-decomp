# jpog.exe: our reimplementation of SimJP.exe. For now (x86) it links against the original engine
# DLLs for every engine function we have not reimplemented yet.
add_executable(jpog WIN32
  AApplication.cpp
  AApplication.h
  AConstantNames.inl
  AConstants.cpp
  AConstants.h
  ADummyTask.cpp
  ADummyTask.h
  AFadeDist.cpp
  AFadeDist.h
  ALocale.cpp
  ALocale.h
  AOptionLogic.cpp
  AOptionLogic.h
  AOptions.cpp
  AOptions.h
  AParticleSystem.cpp
  AParticleSystem.h
  ARenderer.cpp
  ARenderer.h
  ARootTask.cpp
  ARootTask.h
  ASystemTasks.cpp
  ASystemTasks.h
  AUnitFactoryUnitType.cpp
  AUnitFactoryUnitType.h
  AWeatherManager.cpp
  AWeatherManager.h
  CrashHandler.cpp
  CrashHandler.h
  main.cpp
  Memory.cpp
)
# Like SimJP.exe, jpog.exe links the engine modules it uses directly (our kernel DLL replaces the
# original TKernelInterface.dll at run time) and imports their data explicitly.
target_link_libraries(jpog PRIVATE jpog_common original::TKernelInterface original::TApplication original::TRenderInterface
                                   original::TRenderD3DInterface original::TGuiInterface original::TTerrainInterface
                                   original::PPropertyParser
                                   dbghelp)
target_compile_definitions(jpog PRIVATE JPOG_IMPORT_ENGINE_DATA JPOG_IMPORT_PPROPERTYPARSER_DATA)

# The engine loads its modules, shaders and data relative to the working directory, so the game
# runs from the local game copy (gitignored). Copy the exe there after each build when it exists.
if(EXISTS "${JPOG_GAME_DIR}/TKernelInterface.dll")
  add_custom_command(TARGET jpog POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:jpog>" "${JPOG_GAME_DIR}/jpog.exe"
    COMMENT "Copying jpog.exe to ${JPOG_GAME_DIR}")
endif()
