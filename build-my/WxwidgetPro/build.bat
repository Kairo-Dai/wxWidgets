@echo off

rem Simple helper script to configure WxwidgetPro demo project with VS2022 (v143)
setlocal

set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR%"
set "REPO_DIR=%SCRIPT_DIR%/../.."

rem Configure Win32 build using CMake.
rem This expects wxWidgets to have been built and installed into
rem "%SCRIPT_DIR%..\install_win32" (see build-my\build.bat).
cmake -G "Visual Studio 17 2022" -A Win32 ^
  -DwxWidgets_ROOT="%REPO_DIR%\build-my\install_win32" ^
  --fresh -B "%PROJECT_DIR%\build_win32" -S "%PROJECT_DIR%"

endlocal

