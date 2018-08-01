@echo off
setlocal

set IONHOME=%~dp0
set IONOS=win32

pushd %IONHOME%

call bin\vsenv -arch=x64

if exist bin\ion.exe (
  bin\ion.exe -nosourcemap -notypeinfo -o c\out_ion_win32.c ion
) else (
  echo.
  echo -- No ion binary found; using previously built c\out_ion_win32.c --
  echo.
)

cl /Fe:bin\ion /Fo:bin\ion c\ion.c

popd

endlocal
