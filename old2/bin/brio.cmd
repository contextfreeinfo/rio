@echo off

REM Pollute env for faster builds later.
call "%~dp0\vsenv" -arch=x64

setlocal

call :resolve "%~dp0\.." RIOHOME
set RIOOS=win32

if not exist "%RIOHOME%\bin\rio.exe" (
  call "%RIOHOME%\build"
)

"%RIOHOME%\bin\rio.exe" %*

endlocal

goto :eof

:resolve
set %2=%~f1
goto :eof
