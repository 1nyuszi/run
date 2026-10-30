@echo off
setlocal
pushd "%~dp0"
echo Compiling run.cpp...
g++ run.cpp -o run.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation failed.
    pause
    popd
    exit /b 1
)

echo Compiling runui.cpp...
g++ -std=c++11 -Wall -Wextra runui.cpp -o runui.exe -mwindows -static -lgdi32 -luser32
if %ERRORLEVEL% neq 0 (
    echo GUI compilation failed.
    pause
    popd
    exit /b 1
)

echo Compilation successful. Outputs: run.exe and runui.exe
popd
pause
