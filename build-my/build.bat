@echo off

rem Simple helper script to build SDL VisualC solution for Win32 and x64 with VS2022 (v143)
setlocal

set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."

cmake -G "Visual Studio 17 2022" -A Win32 -DCMAKE_INSTALL_PREFIX="%SCRIPT_DIR%\install_win32" ^
-DwxUSE_LIBLZMA=ON -DwxBUILD_MONOLITHIC=ON -DwxBUILD_INSTALL_PDB=ON -DwxBUILD_SAMPLES=ALL -DwxBUILD_DEMOS=ON -DwxUSE_GRAPHICS_DIRECT2D=ON ^
--fresh -B "%SCRIPT_DIR%build_win32" -S "%ROOT_DIR%"

rem cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX="%SCRIPT_DIR%\install_x64" -DwxUSE_LIBLZMA=ON  -B "%SCRIPT_DIR%build_x64"      -S "%ROOT_DIR%"
