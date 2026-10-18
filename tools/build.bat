@echo off
rem Configure, build and test a CMake preset with the MSVC environment.
rem Usage: tools\build.bat [preset] [cmake configure args...]
rem   presets: x86-debug (default), x64-debug, x64-release
rem   e.g. tools\build.bat x86-debug "-DJPOG_REPLACE_KERNEL=ON" (quoted: cmd splits at "=")
rem Visual Studio is located with vswhere, so any VS 2022+ install with the C++ x86/x64 tools works.
setlocal
rem Before the argument loop: shift also moves %0.
set "ROOT=%~dp0.."

set "PRESET=%~1"
if "%PRESET%"=="" set "PRESET=x86-debug"
set "CONFIG_ARGS="
:args
shift
if "%~1"=="" goto argsdone
set "CONFIG_ARGS=%CONFIG_ARGS% %1"
goto args
:argsdone

set "ARCH=x64"
if /I "%PRESET:~0,3%"=="x86" set "ARCH=x64_x86"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo error: vswhere.exe not found; install Visual Studio 2022+ or its Build Tools. 1>&2
    exit /b 1
)
set "VSDIR="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
    echo error: no Visual Studio install with the C++ x86/x64 build tools found. 1>&2
    exit /b 1
)

call "%VSDIR%\VC\Auxiliary\Build\vcvarsall.bat" %ARCH% >nul || exit /b 1
cd /d "%ROOT%" || exit /b 1
cmake --preset "%PRESET%"%CONFIG_ARGS% || exit /b 1
cmake --build --preset "%PRESET%" || exit /b 1
ctest --test-dir "build\%PRESET%" --output-on-failure || exit /b 1
