@echo off

REM Pollute env for faster builds later.
call "%~dp0\vsenv" -arch=x64

setlocal

call :resolve "%~dp0\.." IONHOME
set IONOS=win32

if not exist "%IONHOME%\bin\rio.exe" (
  call "%IONHOME%\build"
)

"%IONHOME%\bin\rio.exe" %*

endlocal

goto :eof

:resolve
set %2=%~f1
goto :eof
