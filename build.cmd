@echo off
setlocal

set IONHOME=%~dp0
set IONOS=win32

pushd %IONHOME%

call bin\vsenv -arch=x64

if exist bin\rio.exe (
  bin\rio.exe -nosourcemap -notypeinfo -o c\out_rio_win32.c rio
) else (
  echo.
  echo -- No rio binary found; using previously built c\out_rio_win32.c --
  echo.
)

cl /Fe:bin\rio /Fo:bin\rio c\rio.c

popd

endlocal
