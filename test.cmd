@echo off
setlocal
call "%~dp0\build"
path %~dp0\bin;%PATH%
pushd tests
call bion -nosourcemap -o c\out_test1_win32.c test1
call bion -nosourcemap -o c\out_test2_win32.c test2
popd
endlocal
