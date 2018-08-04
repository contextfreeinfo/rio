@echo off
setlocal
call "%~dp0\build"
path %~dp0\bin;%PATH%
pushd tests

call brio -nosourcemap -o c\out_test1_win32.c test1
cl /Fe:run\test1 /Fo:run\test1 c\out_test1_win32.c

call brio -nosourcemap -o c\out_test2_win32.c test2
cl /Fe:run\test2 /Fo:run\test2 c\out_test2_win32.c

rem Add -verbose as needed.
call brio -nosourcemap -notypeinfo -o c\out_test3_win32.c test3
cl /Fe:run\test3 /Fo:run\test3 c\out_test3_win32.c

pushd run
.\test1 > out_test1_win32.txt
.\test2 > out_test2_win32.txt
.\test3 > out_test3_win32.txt
popd

popd
endlocal
