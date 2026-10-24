@echo off
echo Compiling run.cpp...
g++ run.cpp -o run.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation failed.
    pause
    exit /b 1
)

echo Compilation successful. Output: run.exe
pause