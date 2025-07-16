@echo off
setlocal ENABLEEXTENSIONS

:: Check for GCC
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] GCC not found. Install MinGW or TDM-GCC and make sure it's in your PATH.
    pause
    exit /b 1
)

:: Compile demon.c to demon.exe
echo [*] Compiling demon.c...
gcc demon.c -o demon.exe -lwinmm

if exist demon.exe (
    echo [+] Build succeeded: demon.exe created.
    echo [!] Running payload... (Only do this in a VM!)
    demon.exe
) else (
    echo [X] Compilation failed. Check for errors in demon.c.
)

pause
