@echo off
rem Configure, build and test a CMake preset: a wrapper around tools\build.py.
rem Usage: tools\build.bat [preset] [cmake configure args...]
rem   e.g. tools\build.bat x86-debug "-DJPOG_REPLACE_KERNEL=ON" (quoted: cmd splits at "=")
python "%~dp0build.py" %*
