@echo off
rem Usage: tools\build.bat <preset>   (x86-hook-debug | x64-debug | x64-release)
rem Sets up the MSVC environment from VS Build Tools 2026 for this process only, then configures + builds.
setlocal
set PRESET=%1
if "%PRESET%"=="" set PRESET=x86-hook-debug
set ARCH=x64
if /I "%PRESET:~0,3%"=="x86" set ARCH=x64_x86
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %ARCH% >nul || exit /b 1
cd /d "%~dp0.."
cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1
